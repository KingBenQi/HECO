#include "ast_opt/ast/Assignment.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/visitor/IVisitor.h"

Assignment::~Assignment() = default;

Assignment::Assignment(std::unique_ptr<AbstractTarget> target,
                       std::unique_ptr<AbstractExpression> value)
    : target(std::move(target)), value(std::move(value)) {}

Assignment::Assignment(const Assignment &other)
    : target(other.target ? other.target->clone() : nullptr), value(other.value ? other.value->clone() : nullptr) {};

Assignment::Assignment(Assignment &&other)
noexcept: target(std::move(other.target)), value(std::move(other.value)) {};

Assignment &Assignment::operator=(const Assignment &other) {
  target = other.target ? other.target->clone() : nullptr;
  value = other.value ? other.value->clone() : nullptr;
  return *this;
}

Assignment &Assignment::operator=(Assignment &&other) noexcept {
  target = std::move(other.target);
  value = std::move(other.value);
  return *this;
}

std::unique_ptr<Assignment> Assignment::clone() const {
  return std::unique_ptr<Assignment>(clone_impl());
}

bool Assignment::hasTarget() const {
  return target!=nullptr;
}

bool Assignment::hasValue() const {
  return value!=nullptr;
}

AbstractTarget &Assignment::getTarget() {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

const AbstractTarget &Assignment::getTarget() const {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

AbstractExpression &Assignment::getValue() {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

const AbstractExpression &Assignment::getValue() const {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

void Assignment::setTarget(std::unique_ptr<AbstractTarget> newTarget) {
  target = std::move(newTarget);
}

void Assignment::setValue(std::unique_ptr<AbstractExpression> newValue) {
  value = std::move(newValue);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////
Assignment *Assignment::clone_impl() const {
  return new Assignment(*this);
}

void Assignment::accept(IVisitor &v) {
  v.visit(*this);
}

AbstractNode::iterator Assignment::begin() {
  return AbstractNode::iterator(std::make_unique<AssignmentIteratorImpl<AbstractNode>>(*this, 0));
}

AbstractNode::const_iterator Assignment::begin() const {
  return AbstractNode::const_iterator(std::make_unique<AssignmentIteratorImpl<const AbstractNode>>(*this, 0));
}

AbstractNode::iterator Assignment::end() {
  return AbstractNode::iterator(std::make_unique<AssignmentIteratorImpl<AbstractNode>>(*this, countChildren()));
}

AbstractNode::const_iterator Assignment::end() const {
  return AbstractNode::const_iterator(std::make_unique<AssignmentIteratorImpl<const AbstractNode>>(*this,
                                                                                                           countChildren()));
}

size_t Assignment::countChildren() const {
  return hasValue() + hasTarget();
}

nlohmann::json Assignment::toJson() const {
  nlohmann::json j;
  j["type"] = getNodeType();
  j["target"] = target ? target->toJson() : "";
  j["value"] = value ? value->toJson() : "";
  return j;
}

std::string Assignment::toString(bool printChildren) const {
  return AbstractNode::toStringHelper(printChildren, {});
}

std::string Assignment::getNodeType() const {
  return "Assignment";
}