#pragma once

#include "tfp/util/Reference.hpp"
#include "model/Expression.hpp"
#include <QString>

// remove when moving implementations into cpp file
#include "model/Node.hpp"

namespace dpsfg {

class Node;

class Connection : public tfp::RefCounted
{
public:
    void setExpression(Expression* expression) { expression_ = expression; }
    Expression* getExpression() const { return expression_; }
    void setTargetNode(Node* node) { outgoingNode_ = node; }
    Node* getTargetNode() const { return outgoingNode_; }

private:
    tfp::Reference<Expression>  expression_;
    tfp::Reference<Node>        outgoingNode_;
};

}