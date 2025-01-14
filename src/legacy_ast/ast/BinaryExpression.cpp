#include "heco/legacy_ast/ast/BinaryExpression.h"
#include <vector>
#include "heco/legacy_ast/ast_parser/Parser.h"
#include "heco/legacy_ast/ast_utilities/IVisitor.h"

BinaryExpression::~BinaryExpression() = default;

BinaryExpression::BinaryExpression(
    std::unique_ptr<AbstractExpression> left, Operator op, std::unique_ptr<AbstractExpression> right)
    : left(std::move(left)), op(op), right(std::move(right))
{}

BinaryExpression::BinaryExpression(const BinaryExpression &other)
    : left(other.left ? other.left->clone(this) : nullptr), op(other.op),
      right(other.right ? other.right->clone(this) : nullptr)
{}

BinaryExpression::BinaryExpression(BinaryExpression &&other) noexcept
    : left(std::move(other.left)), op(other.op), right(std::move(other.right))
{}

BinaryExpression &BinaryExpression::operator=(const BinaryExpression &other)
{
    left = other.left ? other.left->clone(this) : nullptr;
    op = other.op;
    right = other.right ? other.right->clone(this) : nullptr;
    return *this;
}

BinaryExpression &BinaryExpression::operator=(BinaryExpression &&other) noexcept
{
    left = std::move(other.left);
    op = other.op;
    right = std::move(other.right);
    return *this;
}

std::unique_ptr<BinaryExpression> BinaryExpression::clone(AbstractNode *parent_) const
{
    return std::unique_ptr<BinaryExpression>(clone_impl(parent_));
}

bool BinaryExpression::hasLeft() const
{
    return left != nullptr;
}

bool BinaryExpression::hasRight() const
{
    return right != nullptr;
}

AbstractExpression &BinaryExpression::getLeft()
{
    if (hasLeft())
    {
        return *left;
    }
    else
    {
        throw std::runtime_error("Cannot get null left hand side.");
    }
}

const AbstractExpression &BinaryExpression::getLeft() const
{
    if (hasLeft())
    {
        return *left;
    }
    else
    {
        throw std::runtime_error("Cannot get null left hand side.");
    }
}

Operator &BinaryExpression::getOperator()
{
    return op;
}

const Operator &BinaryExpression::getOperator() const
{
    return op;
}

AbstractExpression &BinaryExpression::getRight()
{
    if (hasRight())
    {
        return *right;
    }
    else
    {
        throw std::runtime_error("Cannot get null right hand side.");
    }
}

const AbstractExpression &BinaryExpression::getRight() const
{
    if (hasRight())
    {
        return *right;
    }
    else
    {
        throw std::runtime_error("Cannot get null right hand side.");
    }
}

void BinaryExpression::setLeft(std::unique_ptr<AbstractExpression> newLeft)
{
    left = std::move(newLeft);
}

void BinaryExpression::setOperator(Operator newOperator)
{
    op = newOperator;
}

void BinaryExpression::setRight(std::unique_ptr<AbstractExpression> newRight)
{
    right = std::move(newRight);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////

BinaryExpression *BinaryExpression::clone_impl(AbstractNode *parent_) const
{
    auto p = new BinaryExpression(*this);
    if (parent_)
    {
        p->setParent(*parent_);
    }
    return p;
}

void BinaryExpression::accept(IVisitor &v)
{
    v.visit(*this);
}

AbstractNode::iterator BinaryExpression::begin()
{
    return AbstractNode::iterator(std::make_unique<BinaryExpressionIteratorImpl<AbstractNode>>(*this, 0));
}

AbstractNode::const_iterator BinaryExpression::begin() const
{
    return AbstractNode::const_iterator(std::make_unique<BinaryExpressionIteratorImpl<const AbstractNode>>(*this, 0));
}

AbstractNode::iterator BinaryExpression::end()
{
    return AbstractNode::iterator(std::make_unique<BinaryExpressionIteratorImpl<AbstractNode>>(*this, countChildren()));
}

AbstractNode::const_iterator BinaryExpression::end() const
{
    return AbstractNode::const_iterator(
        std::make_unique<BinaryExpressionIteratorImpl<const AbstractNode>>(*this, countChildren()));
}

size_t BinaryExpression::countChildren() const
{
    return size_t(hasLeft()) + hasRight();
}

nlohmann::json BinaryExpression::toJson() const
{
    nlohmann::json j = { { "type", getNodeType() } };
    if (hasLeft())
        j["left"] = getLeft().toJson();
    j["operator"] = op.toString();
    if (hasRight())
        j["right"] = getRight().toJson();

    return j;
}

std::unique_ptr<BinaryExpression> BinaryExpression::fromJson(nlohmann::json j)
{
    // Parse operation
    auto op = fromStringToOperatorVariant(j["operator"]);

    // Parse operands (if available)
    std::unique_ptr<AbstractExpression> left, right;

    if (j.find("left") != j.end())
        left = Parser::parseJsonExpression(j["left"]);

    if (j.find("right") != j.end())
        right = Parser::parseJsonExpression(j["right"]);

    return std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
}

std::string BinaryExpression::toString(bool printChildren) const
{
    return AbstractNode::toStringHelper(printChildren, { op.toString() });
}
std::string BinaryExpression::getNodeType() const
{
    return "BinaryExpression";
}
