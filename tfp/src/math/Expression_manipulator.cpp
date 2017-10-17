#include "tfp/math/Expression.hpp"
#include "tfp/math/VariableTable.hpp"
#include "tfp/math/CoefficientPolynomial.hpp"
#include "tfp/math/TransferFunction.hpp"
#include <cstring>

using namespace tfp;

// ----------------------------------------------------------------------------
void Expression::enforceProductLHS(const char* variable)
{
    if (left())  left()->enforceProductLHS(variable);
    if (right()) right()->enforceProductLHS(variable);
    if (isOperation(op::mul))
    {
        if (right() && right()->hasVariable(variable))
        {
            tfp::Reference<Expression> tmp = right_;
            right_ = left_;
            left_ = tmp;
        }
    }
}

// ----------------------------------------------------------------------------
bool Expression::enforceConstantExponent(const char* variable)
{
    /*
     * Performs the following transformations:
     *    s    --> s^1
     * If s is raised to a non-constant (e.g. s^a or (s+2)^a) abort and return false.
     */
    bool success = true;
    if (left())  success &= left()->enforceConstantExponent(variable);
    if (right()) success &= right()->enforceConstantExponent(variable);
    if (hasVariable(variable) == false)
        return success;
    
    // Travel up the tree in search of op::pow...
    for (Expression* e = parent(); e != NULL; e = e->parent())
        if (e->isOperation(op::pow))
            if (e->right()->type() != CONSTANT)  // RHS has to be constant
                return false;
    
    // If immediate parent is already pow (and has a constant RHS), we're done.
    if (parent()->isOperation(op::pow) == true)
        return true;
    
    set (op::pow, Expression::make(this), Expression::make(1.0));
    return success;
}

// ----------------------------------------------------------------------------
bool Expression::eliminateDivisionsAndSubtractions(const char* variable)
{
    /*
     * Only manipulate branches that are an ancestor of an expression with the
     * specified variable.
     */
    bool weMatter = false;
    if (left())  weMatter |= left()->eliminateDivisionsAndSubtractions(variable);
    if (right()) weMatter |= right()->eliminateDivisionsAndSubtractions(variable);
    if (weMatter == false && hasVariable(variable) == false)
        return false;

    /*
     * Performs the following transformations:
     *   a/s    -->  as^-1
     *   a/s^x  -->  as^-x
     *   a-s    -->  a+s*(-1)
     *   a-sx   -->  a+s*(-x)
     * This makes the tree easier to work with for later stages, because
     * the assumption that no division or subtraction operators exist can be
     * made.
     */
    static struct { op::Op2 outer; op::Op2 outerInv; op::Op2 inner; } ops[2] = {
        { op::div, op::mul, op::pow },
        { op::sub, op::add, op::mul }
    };

    for (int i = 0; i != 2; ++i)
    {
        if (isOperation(ops[i].outer) == false)
            continue;
        
        if (hasRHSOperation(ops[i].inner))
        {
            Expression* toNegate = right()->right();
            set(ops[i].outerInv, left(), right());
            if (toNegate->type() == CONSTANT)
                toNegate->set(-toNegate->value());
            else
                toNegate->set(op::negate, Expression::make(toNegate));

        }
        else
        {
            set(ops[i].outerInv,
                left(),
                Expression::make(ops[i].inner,
                    right(),
                    Expression::make(-1)));
        }
    }

    root()->dump();
    return true;
}

// ----------------------------------------------------------------------------
bool Expression::heaveSums(const char* variable)
{
    /*
     * Only manipulate branches that are an ancestor of an expression with the
     * specified variable.
     */
    bool weMatter = false;
    if (left())  weMatter |= left()->heaveSums(variable);
    if (right()) weMatter |= right()->heaveSums(variable);
    if (weMatter == false && hasVariable(variable) == false)
        return false;
    
    if (isOperation(op::add) == false)
        return weMatter;
}

// ----------------------------------------------------------------------------
bool Expression::manipulateIntoRationalFunction(const char* variable)
{
    /*
     * Create the "split" operator, i.e. this is the expression that splits the
     * numerator from the denominator. By default, use the last division
     * operator we can find in the tree. If none exists, create a div operator
     * that has no effect.
     */
    Expression* split = find(op::div);
    if (split == NULL)
        split = Expression::make(op::div, split, Expression::make(1.0));

    /*
     * Eliminate divisions and subtractions on both sides of the split, so we
     * can start shuffling terms back and forth between the numerator and
     * denominator without having to deal with lots of edge cases.
     */
    split->left()->eliminateDivisionsAndSubtractions(variable);
    split->right()->eliminateDivisionsAndSubtractions(variable);
    
    /*
     * All op::pow operations with our variable on the LHS are guaranteed
     * to have a constant RHS. If not, error out.
     */
    if (enforceConstantExponent(variable) == false)
        return false;
        //throw std::runtime_error("This expression has variable exponents! These cannot be reduced to a rational function.");
    
    
}

// ----------------------------------------------------------------------------
Expression::TransferFunctionCoefficients Expression::calculateTransferFunctionCoefficients(const char* variable)
{
    
    
    return TransferFunctionCoefficients();
}

// ----------------------------------------------------------------------------
tfp::TransferFunction<double>
Expression::calculateTransferFunction(const TransferFunctionCoefficients& tfe,
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