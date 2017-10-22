#pragma once

#include "tfp/config.hpp"
#include "tfp/util/Reference.hpp"
#include <string>
#include <set>

namespace tfp {

class VariableTable;

namespace op {

typedef double (*Op1)(double);
typedef double (*Op2)(double,double);
TFP_PUBLIC_API double add(double a, double b);
TFP_PUBLIC_API double sub(double a, double b);
TFP_PUBLIC_API double mul(double a, double b);
TFP_PUBLIC_API double div(double a, double b);
TFP_PUBLIC_API double pow(double a, double b);
TFP_PUBLIC_API double mod(double a, double b);
TFP_PUBLIC_API double negate(double a);
TFP_PUBLIC_API double comma(double a, double b);

}

class TFP_PUBLIC_API Expression : public tfp::RefCounted
{
public:
    enum Type
    {
        INVALID,
        CONSTANT,
        VARIABLE,
        FUNCTION1,
        FUNCTION2
    };

    Expression();
    ~Expression();

    static Expression* parse(const char* expression);

    static Expression* make(const char* variableName);
    static Expression* make(double value);
    static Expression* make(op::Op1 func, Expression* rhs);
    static Expression* make(op::Op2 func, Expression* lhs, Expression* rhs);

    void copyDataFrom(const Expression* other);
    void stealDataFrom(Expression* other);
    Expression* swapWith(Expression* other);
    // deep copy, parent of root node defaults to NULL
    Expression* clone(Expression* parent=NULL) const;
    void swapOperands();

    void collapseIntoParent();
    void set(const char* variableName);
    void set(double value);
    void set(op::Op1 func, Expression* rhs);
    void set(op::Op2 func, Expression* lhs, Expression* rhs);
    void reset();
    bool optimise();
    
    Expression* find(const char* variableName);
    Expression* find(double value);
    Expression* find(op::Op1 func);
    Expression* find(op::Op2 func);

    /*!
     * Recursively finds an expression node of type FUNCTION2 that has at least
     * one operand that matches the specified type. If ignore is not NULL,
     * then the returned expression is guaranteed not to be ignore.
     */
    Expression* findLorR(op::Op2 func, Type LorR, const Expression* ignore=NULL);
    Expression* findSame(const Expression* match);
    Expression* findOpWithSameLHS(op::Op2 func, const Expression* match);
    Expression* findOpWithNegativeRHS(op::Op2 func);
    Expression* travelUpChain(op::Op2 func);
    Expression* findSameDownChain(op::Op2 func, const Expression* match);

    VariableTable* generateVariableTable() const;
    double evaluate(const VariableTable* vt=NULL, std::set<std::string>* visited=NULL) const;
    
    bool checkParentConsistencies() const;

    /*!
     * Returns true if the types match and (depending on type) symbol names, value, or
     * operations match. Invalid expressions return false.
     */
    bool isSameAs(const Expression* other) const;
    bool isOperation(op::Op1 func) const;
    bool isOperation(op::Op2 func) const;
    bool hasRHSOperation(op::Op1 func) const;
    bool hasRHSOperation(op::Op2 func) const;
    bool hasVariable(const char* variable) const;
    Expression* getOtherOperand() const;

    Expression* root() {
        Expression* root = this;
        while (root->parent_ != NULL)
            root = root->parent_;
        return root;
    }
    Expression* parent() const { return parent_; }
    Expression* left() const { return left_; }
    Expression* right() const { return right_; }
    Type type() const { return type_; }
    const char* name() const { assert(type_ == VARIABLE); return data_.name_; }
    double value() const { assert(type_ == CONSTANT); return data_.value_; }
    op::Op1 op1() const { assert(type_ == FUNCTION1); return data_.op1_; }
    op::Op2 op2() const { assert(type_ == FUNCTION2); return data_.op2_; }

    void dump(const char* filename, bool append=false);
    void dump(FILE* fp);
    void dump();

private:

    union {
        double value_;
        char* name_;
        op::Op1 op1_;
        op::Op2 op2_;
    } data_;
    Expression* parent_;
    tfp::Reference<Expression> left_;
    tfp::Reference<Expression> right_;
    Type type_;
};

}
