#include "tfp/math/ExpressionManipulators.hpp"
#include "tfp/math/ExpressionOptimiser.hpp"
#include "tfp/math/Expression.hpp"
#include "tfp/math/VariableTable.hpp"
#include "tfp/math/CoefficientPolynomial.hpp"
#include "tfp/math/TransferFunction.hpp"
#include <cstring>

using namespace tfp;

// ----------------------------------------------------------------------------
bool TFManipulator::factorNegativeExponentsToNumerator(Expression* e, Expression* numerator, const char* variable)
{
    if (e->isOperation(op::mul))
    {
        return factorNegativeExponentsToNumerator(e->left(), numerator, variable) |
               factorNegativeExponentsToNumerator(e->right(), numerator, variable);
    }

    if (e->isOperation(op::pow) == false)
        return false;

    if (e->right()->type() != Expression::CONSTANT || e->right()->value() >= 0.0)
        return false;

    e->right()->set(-e->right()->value());
    factorIn(numerator, e);
    e->getOtherOperand()->collapseIntoParent();
    return true;
}

// ----------------------------------------------------------------------------
inline bool logically_equal(double a, double b, double error_factor=1.0)
{
  return a==b ||
    std::abs(a-b)<std::abs(std::min(a,b))*std::numeric_limits<double>::epsilon()*
                  error_factor;
}
bool TFManipulator::manipulateIntoRationalFunction(Expression* e, const char* variable)
{
    /*
     * Create the "split" operator, i.e. this is the expression that splits the
     * numerator from the denominator. By default, use the last division
     * operator we can find in the tree. If none exists, create a div operator
     * that has no effect.
     */
    Expression* split = e->type() == Expression::FUNCTION2 && e->op2() == op::div ?
            e : Expression::make(op::div, e, Expression::make(1.0));

    /*
     * Eliminate divisions and subtractions on both sides of the split, so we
     * can start shuffling terms back and forth between the numerator and
     * denominator without having to deal with lots of edge cases.
     */
    recursivelyCall(&TFManipulator::eliminateDivisionsAndSubtractions, split->left(), variable);
    recursivelyCall(&TFManipulator::eliminateDivisionsAndSubtractions, split->right(), variable);

    /*
     * All op::pow operations with our variable on the LHS need to have a
     * constant RHS. If not, error out, because such an expression cannot be
     * reduced to a rational function.
     */
    /*if (enforceConstantExponent(variable) == false)
        return false;*/
        //throw std::runtime_error("This expression has variable exponents! These cannot be reduced to a rational function.");
    ExpressionOptimiser optimise;
    while (true)
    {
        while (recursivelyCall(&TFManipulator::expandConstantExponentsIntoProducts, split->right(), variable)) {}
        while (recursivelyCall(&TFManipulator::expand, split->right(), variable)) {}
        optimise.constants(split->right());
        optimise.uselessOperations(split->right());

        while (recursivelyCall(&TFManipulator::factorNegativeExponents, split->right(), variable)) {}

        if (factorNegativeExponentsToNumerator(split->right(), split->left(), variable) == false)
            break;
    }

    while (optimise.uselessOperations(split->left()) |
           recursivelyCall(&TFManipulator::expandConstantExponentsIntoProducts, split, variable) |
           recursivelyCall(&TFManipulator::expand, split, variable))
    {}

    optimise.everything(e);

    return false;
}

// ----------------------------------------------------------------------------
typedef std::pair< int, Reference<Expression> > CoeffExp;
typedef std::vector<CoeffExp> UnsortedCoeffs;
static bool coeff(UnsortedCoeffs* coeffs, Expression* e, const char* variable)
{
    if (e->hasVariable(variable) == false)
        return false;

    // If parent is power operator, its RHS is the coefficient degree. Otherwise
    // assume degree of 1
    CoeffExp coeff;
    if (e->parent()->isOperation(op::pow))
    {
        if (e->parent()->right()->type() != Expression::CONSTANT)
            throw std::runtime_error("Exptected a constant degree!");
        coeff.first = (int)e->parent()->right()->value();
    }
    else
        coeff.first = 1;

    // Travel up the chain of multiplications to get the complete coefficient expression
    e = e->parent()->isOperation(op::pow) ? e->parent() : e;
    Expression* top = e;
    while (top->parent() && top->parent()->isOperation(op::mul))
        top = top->parent();
    coeff.second = top;
    coeffs->push_back(coeff);

    if (top->parent() == NULL)
        return false;

    e->getOtherOperand()->collapseIntoParent();
    top->getOtherOperand()->collapseIntoParent();
    return true;
}
static bool coeffsPow(UnsortedCoeffs* coeffs, Expression* e, const char* variable)
{
    if (e->isOperation(op::pow))
    {
        return coeff(coeffs, e->left(), variable);
    }
    else
    {
        return coeff(coeffs, e, variable);
    }
}
static bool coeffsMul(UnsortedCoeffs* coeffs, Expression* e, const char* variable)
{
    if (e->isOperation(op::mul))
    {
        return coeffsMul(coeffs, e->right(), variable) ||
               coeffsMul(coeffs, e->left(), variable);
    }
    else
    {
        return coeffsPow(coeffs, e, variable);
    }
}
static bool coeffsAdd(UnsortedCoeffs* coeffs, Expression* e, const char* variable)
{
    if (e->isOperation(op::add))
    {
        return coeffsAdd(coeffs, e->right(), variable) ||
               coeffsAdd(coeffs, e->left(), variable);
    }
    else
    {
        return coeffsMul(coeffs, e, variable);
    }
}

TFManipulator::TFCoefficients
TFManipulator::calculateTransferFunctionCoefficients(Expression* e, const char* variable)
{
    manipulateIntoRationalFunction(e, variable);
    e->dump("wtf.dot", true);

    Reference<Expression> numerator = e->left();
    Reference<Expression> denominator = e->right();
    numerator->unlinkFromTree();
    denominator->unlinkFromTree();

    UnsortedCoeffs num;
    UnsortedCoeffs den;
    numerator->dump("wtf.dot", true);
    while (coeffsAdd(&num, numerator, variable)) {
        numerator->dump("wtf.dot", true); }
    denominator->dump("wtf.dot", true);
    while (coeffsAdd(&den, denominator, variable)) {
        denominator->dump("wtf.dot", true); }

    return TFCoefficients();
}

// ----------------------------------------------------------------------------
tfp::TransferFunction<double>
TFManipulator::calculateTransferFunction(const TFCoefficients& tfe,
                                         const VariableTable* vt)
{
    tfp::CoefficientPolynomial<double> numerator(tfe.numeratorCoefficients_.size());
    tfp::CoefficientPolynomial<double> denominator(tfe.denominatorCoefficients_.size());

    for (std::size_t i = 0; i != tfe.numeratorCoefficients_.size(); ++i)
    {
        double value = tfe.numeratorCoefficients_[i]->evaluate(vt);
        numerator.setCoefficient(i, value);
    }

    for (std::size_t i = 0; i != tfe.denominatorCoefficients_.size(); ++i)
    {
        double value = tfe.denominatorCoefficients_[i]->evaluate(vt);
        numerator.setCoefficient(i, value);
    }

    return tfp::TransferFunction<double>(numerator, denominator);
}
