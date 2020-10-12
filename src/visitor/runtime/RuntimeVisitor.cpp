#include <memory>
#include <utility>
#include <iostream>

#include "ast_opt/visitor/runtime/RuntimeVisitor.h"
#include "ast_opt/ast/BinaryExpression.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/Call.h"
#include "ast_opt/ast/ExpressionList.h"
#include "ast_opt/ast/For.h"
#include "ast_opt/ast/Function.h"
#include "ast_opt/ast/If.h"
#include "ast_opt/ast/IndexAccess.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/OperatorExpression.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/ast/UnaryExpression.h"
#include "ast_opt/ast/Assignment.h"
#include "ast_opt/ast/VariableDeclaration.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/visitor/runtime/AbstractCiphertextFactory.h"
#include "ast_opt/parser/Tokens.h"

std::unique_ptr<AbstractValue> SpecialRuntimeVisitor::getNextStackElement() {
  auto elem = std::move(intermedResult.top());
  intermedResult.pop();
  return elem;
}

void SpecialRuntimeVisitor::visit(BinaryExpression &elem) {
  // ---- some helper methods -------------------------
  auto operatorEqualsAnyOf = [&elem](std::initializer_list<OperatorVariant> op) -> bool {
    return std::any_of(op.begin(), op.end(), [&elem](OperatorVariant op) { return elem.getOperator()==Operator(op); });
  };
  auto operatorEquals = [&elem](OperatorVariant op) -> bool {
    return elem.getOperator()==Operator(op);
  };
  auto isSecretTainted = [&](const std::string &uniqueNodeId) -> bool {
    // we assume here that if it is NOT in the map, then it is NOT secret tainted
    return (secretTaintedMap.count(uniqueNodeId) > 0 && secretTaintedMap.at(uniqueNodeId));
  };
  // ---- end

  auto lhsIsSecret = isSecretTainted(elem.getLeft().getUniqueNodeId());
  auto rhsIsSecret = isSecretTainted(elem.getRight().getUniqueNodeId());

  // if lhs or rhs are secret tainted but the operator is non-FHE compatible, throw an exception
  if ((lhsIsSecret || rhsIsSecret) && !operatorEqualsAnyOf({FHE_ADDITION, FHE_SUBTRACTION, FHE_MULTIPLICATION})) {
    throw std::runtime_error("An operand in the binary expression is a ciphertext but given operation ("
                                 + elem.getOperator().toString() + ") cannot be executed on ciphertexts using FHE!\n"
                                 + "Expression: " + elem.toString(false));
  }

  elem.getLeft().accept(*this);
  auto lhsOperand = getNextStackElement();

  elem.getRight().accept(*this);
  auto rhsOperand = getNextStackElement();

  // if exactly one of the operands is a ciphertext and we have a commutative operation, then we make sure that
  // the first operand (the one we call the operation on) is the ciphertext as otherwise it will fail
  if ((lhsIsSecret!=rhsIsSecret) && elem.getOperator().isCommutative()) {
    if (rhsIsSecret) std::swap(lhsOperand, rhsOperand);
  }

  // TODO: Implement me!
  if (operatorEqualsAnyOf({ADDITION, FHE_ADDITION})) {
    lhsOperand->add(*rhsOperand);
    intermedResult.push(std::move(lhsOperand));
  } else if (operatorEqualsAnyOf({SUBTRACTION, FHE_SUBTRACTION})) {

  } else if (operatorEqualsAnyOf({MULTIPLICATION, FHE_MULTIPLICATION})) {

  } else if (operatorEquals(DIVISION)) {

  } else if (operatorEquals(MODULO)) {

  } else if (operatorEquals(LOGICAL_AND)) {

  } else if (operatorEquals(LOGICAL_OR)) {

  } else if (operatorEquals(LESS)) {

  } else if (operatorEquals(LESS_EQUAL)) {

  } else if (operatorEquals(GREATER)) {

  } else if (operatorEquals(GREATER_EQUAL)) {

  } else if (operatorEquals(EQUAL)) {

  } else if (operatorEquals(NOTEQUAL)) {

  } else if (operatorEquals(BITWISE_AND)) {

  } else if (operatorEquals(BITWISE_XOR)) {

  } else if (operatorEquals(BITWISE_OR)) {

  }
}

void SpecialRuntimeVisitor::visit(UnaryExpression &elem) {
  ScopedVisitor::visit(elem);



  // TODO: Implement me!
  if (elem.getOperator()==Operator(LOGICAL_NOT)) {

  } else if (elem.getOperator()==Operator(BITWISE_NOT)) {

  } else {
    throw std::runtime_error("Unknown unary operator encountered!");
  }
}

void SpecialRuntimeVisitor::visit(Block &elem) {
  ScopedVisitor::visit(elem);
}

void SpecialRuntimeVisitor::visit(Call &elem) {
  if (elem.getIdentifier()!=stork::to_string(stork::reservedTokens::kw_rotate)) {
    throw std::runtime_error("Calls other than 'rotate(identifier: label, numSteps: int);' are not supported yet!");
  }

  // handle 'rotate' instruction
  if (elem.getArguments().size() < 2) {
    throw std::runtime_error(
        "Instruction 'rotate' requires two arguments: (1) identifier of ciphertext to be rotated "
        "and the (2) number of steps to rotate the ciphertext.");
  }

  // arg 0: ciphertext to rotate
  auto ciphertextIdentifier = elem.getArguments().at(0);
  std::unique_ptr<AbstractCiphertext> ctxt;
  auto ciphertextIdentifierVariable = dynamic_cast<Variable *>(&ciphertextIdentifier.get());
  if (ciphertextIdentifierVariable==nullptr) {
    throw std::runtime_error("Argument 'ciphertext' in 'rotate' instruction must be a variable.");
  }
  auto scopedIdentifier = getCurrentScope().resolveIdentifier(ciphertextIdentifierVariable->getIdentifier());

  // arg 1: rotation steps
  auto steps = elem.getArguments().at(1);
  auto stepsLiteralInt = dynamic_cast<LiteralInt *>(&steps.get());
  if (stepsLiteralInt==nullptr) {
    throw std::runtime_error("Argument 'steps' in 'rotate' instruction must be an integer.");
  }

  // perform rotation
  declaredCiphertexts.at(scopedIdentifier)->rotateRowsInplace(stepsLiteralInt->getValue());
}

void SpecialRuntimeVisitor::visit(ExpressionList &elem) {
  ScopedVisitor::visit(elem);
  // after visiting the expression list with this visitor, there should only be Literals left, otherwise this expression
  // list is not valid; we use here that the TypeCheckingVisitor verified that all expressions in an ExpressionList have
  // the same type

  std::vector<std::unique_ptr<ICleartext>> cleartextVec;
  for (size_t i = 0; i < elem.getExpressions().size(); ++i) {
    auto e = getNextStackElement();
    if (dynamic_cast<ICleartext *>(e.get())) {
      std::unique_ptr<ICleartext> derivedPointer(dynamic_cast<ICleartext *>(e.release()));
      // We are now processing elements of the ExpressionList in reverse order, i.e., the last visited element of the
      // ExpressionList is on the top of the stack. Thus, we need to append elements in the cleartextVec to the front.
      cleartextVec.insert(cleartextVec.begin(), std::move(derivedPointer));
    } else {
      throw std::runtime_error("");
    }
  }

  auto firstExpression = elem.getExpressions().at(0);
  if (dynamic_cast<LiteralBool *>(&firstExpression.get())) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralBool::value_type>>(cleartextVec));
  } else if (dynamic_cast<LiteralChar *>(&elem)) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralChar::value_type>>(cleartextVec));
  } else if (dynamic_cast<LiteralInt *>(&firstExpression.get())) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralInt::value_type>>(cleartextVec));
  } else if (dynamic_cast<LiteralFloat *>(&firstExpression.get())) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralFloat::value_type>>(cleartextVec));
  } else if (dynamic_cast<LiteralDouble *>(&firstExpression.get())) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralDouble::value_type>>(cleartextVec));
  } else if (dynamic_cast<LiteralString *>(&firstExpression.get())) {
    intermedResult.emplace(std::make_unique<Cleartext<LiteralString::value_type>>(cleartextVec));
  } else {
    throw std::runtime_error("Could not determine element type of ExpressionList!");
  }
}

void SpecialRuntimeVisitor::visit(For &elem) {
  elem.getInitializer().accept(*this);

  // a helper method to check the value of the For loop's condition
  auto evaluateCondition = [&](AbstractExpression &expr) -> bool {
    expr.accept(*this);
    auto result = getNextStackElement();
    if (auto conditionLiteralBool = dynamic_cast<LiteralBool *>(result.get())) {
      return conditionLiteralBool->getValue();
    } else {
      throw std::runtime_error("For loop's condition must be evaluable to a Boolean.");
    }
  };

  // TODO: Do we support loop's over a secret condition? If not, include check in elem.hasCondition() to make sure that
  //  the condition is not secret -> otherwise throw exception

  // execute the For loop
  if (elem.hasCondition()) {
    while (evaluateCondition(elem.getCondition())) {
      if (elem.hasBody()) elem.getBody().accept(*this);
      if (elem.hasUpdate()) elem.getUpdate().accept(*this);
    }
  } else {
    throw std::runtime_error("For loops without a condition are not supported yet!");
  }
}

void SpecialRuntimeVisitor::visit(Function &) {
  throw std::runtime_error("Function statements are not supported yet by RuntimeVisitor.");
}

void SpecialRuntimeVisitor::visit(If &elem) {
  // check if the If statement's condition is secret
  // (although we ran the SecretBranchingVisitor before, it can be that there are still secret conditions left,
  // for example, if the then/else branch contains an unsupported statement such as a loop)
  if (secretTaintedMap.at(elem.getCondition().getUniqueNodeId())) {
    throw std::runtime_error("If statements over secret conditions that cannot be rewritten using the "
                             "SecretBranchingVisitor are not supported yet!");
  }

  // get the If statement's condition
  elem.getCondition().accept(*this);
  auto conditionResult = getNextStackElement();

  if (auto conditionLiteralBool = dynamic_cast<Cleartext<bool> *>(conditionResult.get())) {
    if (conditionLiteralBool->getData().front()) {
      // visit "then" branch
      elem.getThenBranch().accept(*this);
    } else if (elem.hasElseBranch()) {
      // visit "else" branch if existent
      elem.getElseBranch().accept(*this);
    }
  } else {
    throw std::runtime_error("Condition of If statement must be evaluable to a bool.");
  }
}

void SpecialRuntimeVisitor::visit(IndexAccess &elem) {
  ScopedVisitor::visit(elem);
  // TODO: Implement me!

  elem.getTarget().accept(*this);

  elem.getIndex().accept(*this);
}

void SpecialRuntimeVisitor::visit(LiteralBool &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<bool>>(elem));
}

void SpecialRuntimeVisitor::visit(LiteralChar &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<char>>(elem));
}

void SpecialRuntimeVisitor::visit(LiteralInt &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<int>>(elem));
}

void SpecialRuntimeVisitor::visit(LiteralFloat &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<float>>(elem));
}

void SpecialRuntimeVisitor::visit(LiteralDouble &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<double>>(elem));
}

void SpecialRuntimeVisitor::visit(LiteralString &elem) {
  ScopedVisitor::visit(elem);
  intermedResult.emplace(std::make_unique<Cleartext<std::string>>(elem));
}

void SpecialRuntimeVisitor::visit(OperatorExpression &) {
  throw std::runtime_error("UNIMPLEMENTED: RuntimeVisitor cannot execute OperatorExpressions yet.");
}

void SpecialRuntimeVisitor::visit(Return &elem) {
  ScopedVisitor::visit(elem);
  throw std::runtime_error("UNSUPPORT: RuntimeVisitor can neither break out of program with a return statement nor "
                           "does return specified values as program's output.");
}

void SpecialRuntimeVisitor::visit(Assignment &elem) {

  elem.getTarget().accept(*this);
  // auto &assignmentTarget = getNextStackElement();

//  //
//  if (auto atVariable = dynamic_cast<Variable *>(&assignmentTarget)) {
//    assignmentTargetIdentifiers.emplace_back(getCurrentScope(), atVariable->getIdentifier());
//  } else if (auto atIndexAccess = dynamic_cast<IndexAccess *>(&assignmentTarget)) {
//    // after visiting the target, the index access should not contain any nested index accesses anymore,
//    // for example, i[k[2]] -> i[4] assuming k[2] = 4
//    if (auto indexAccessVariable = dynamic_cast<Variable *>(&atIndexAccess->getTarget())) {
//      assignmentTargetIdentifiers.emplace_back(getCurrentScope(), indexAccessVariable->getIdentifier());
//    } else {
//      throw std::runtime_error("");
//    }
//  }

  elem.getValue().accept(*this);


  // TODO: Implement me!
}

std::vector<int64_t> extractIntegerFromLiteralInt(ExpressionList &el) {
  std::vector<int64_t> result;
  for (std::reference_wrapper<AbstractExpression> &abstractExpr : el.getExpressions()) {
    if (auto casted = dynamic_cast<LiteralInt *>(&abstractExpr.get())) {
      result.push_back(casted->getValue());
    } else {
      throw std::runtime_error("Elements of ExpressionList are expected to be LiteralInts, "
                               "found " + std::string(typeid(abstractExpr).name()) + " instead.");
    }
  }
  return result;
}

void SpecialRuntimeVisitor::visit(VariableDeclaration &elem) {
  auto scopedIdentifier = std::make_unique<ScopedIdentifier>(getCurrentScope(), elem.getTarget().getIdentifier());
  identifierDatatypes.emplace(*scopedIdentifier, elem.getDatatype());
  getCurrentScope().addIdentifier(std::move(scopedIdentifier));

  // if this declaration does not have an initialization, we can stop here as there's no value we need to keep track of
  if (!elem.hasValue()) return;

  // after having visited the variable declaration's initialization value, then we should have a Cleartext<T> on the
  // top of the intermedResult stack
  elem.getValue().accept(*this);
  std::unique_ptr<AbstractValue> initializationValue = getNextStackElement();

  if (elem.getDatatype().getSecretFlag()) {
    // declaration of a secret variable: we need to create a ciphertext here
    auto sident = std::make_unique<ScopedIdentifier>(getCurrentScope(), elem.getTarget().getIdentifier());
    auto ctxt = factory.createCiphertext(std::move(initializationValue));
    declaredCiphertexts.insert_or_assign(*sident, std::move(ctxt));
  } else if (dynamic_cast<ICleartext *>(initializationValue.get())) {
    // declaration of a non-secret variable
    // we need to convert std::unique_ptr<AbstractValue> from intermedResult into std::unique_ptr<ICleartext>
    // as we now that this variable declaration's value is not secret tainted, this must be a cleartext
    auto sident = std::make_unique<ScopedIdentifier>(getCurrentScope(), elem.getTarget().getIdentifier());
    std::unique_ptr<ICleartext> cleartextUPtr(dynamic_cast<ICleartext *>(initializationValue.release()));
    declaredCleartexts.insert_or_assign(*sident, std::move(cleartextUPtr));
  } else {
    throw std::runtime_error("Initialization value of VariableDeclaration ( " + elem.getTarget().getIdentifier()
                                 + ") could not be processed successfully.");
  }
}

void SpecialRuntimeVisitor::visit(Variable &elem) {
  auto scopedIdentifier = getCurrentScope().resolveIdentifier(elem.getIdentifier());
  // in both cases we need to clone the underlying type (AbstractCiphertext or Cleartext) as the maps
  // (declaredCiphertexts and declaredCleartexts) holds ownership and it could be that the same variable will be
  // referenced later again
  if (identifierDatatypes.at(scopedIdentifier).getSecretFlag()) {
    // variable refers to an encrypted value, i.e., is a ciphertext
    auto clonedCiphertext = declaredCiphertexts.at(scopedIdentifier)->clone();
    intermedResult.emplace(std::move(clonedCiphertext));
  } else {
    // variable refers to a cleartext value
    auto clonedCleartext = declaredCleartexts.at(scopedIdentifier)->clone();
    intermedResult.emplace(std::move(clonedCleartext));
  }
}

template<typename T>
void SpecialRuntimeVisitor::checkAstStructure(AbstractNode &astRootNode) {
  /// The input and output ASTs are expected to consist of a single block with variable declaration
  /// statements and variable assignments, respectively.

  if (dynamic_cast<Block *>(&astRootNode)==nullptr)
    throw std::runtime_error("Root of (in-/out)put AST must be a Block node.");

  // check each statement of the AST
  for (auto &statement : astRootNode) {
    // check that statements of Block are of expected type
    auto castedStatement = dynamic_cast<T *>(&statement);
    if (castedStatement==nullptr) {
      throw std::runtime_error("Block statements of given (in-/out)put AST must be of type "
                                   + std::string(typeid(T).name()) + ". ");
    }

    // special condition for assignments: require that assignment's value is a Variable or IndexAccess
    if (typeid(T)==typeid(Assignment)) {
      auto valueAsVariable = dynamic_cast<Variable *>(&castedStatement->getTarget());
      auto valueAsIndexAccess = dynamic_cast<IndexAccess *>(&castedStatement->getTarget());
      if (valueAsVariable==nullptr && valueAsIndexAccess==nullptr) {
        throw std::runtime_error("Output AST must consist of Assignments to variables, i.e., Variable or IndexAccess.");
      }
    }
  }
}

SpecialRuntimeVisitor::SpecialRuntimeVisitor(AbstractCiphertextFactory &factory,
                                             AbstractNode &inputs,
                                             SecretTaintedNodesMap &secretTaintedNodesMap)
    : factory(factory), secretTaintedMap(secretTaintedNodesMap) {
  // generate ciphertexts for inputs
  checkAstStructure<VariableDeclaration>(inputs);
  inputs.accept(*this);
}

OutputIdentifierValuePairs SpecialRuntimeVisitor::getOutput(AbstractNode &outputAst) {
  // make sure that outputAst consists of a Block with Assignment statements
  checkAstStructure<Assignment>(outputAst);

  // extract lhs and rhs of assignment
  OutputIdentifierValuePairs outputValues;
  auto block = dynamic_cast<Block &>(outputAst);
  for (auto &assignm : block.getStatements()) {
    // extract assignment's target (lhs)
    auto varAssignm = dynamic_cast<Assignment &>(assignm.get());
    auto identifier = dynamic_cast<Variable &>(varAssignm.getTarget()).getIdentifier();

    // extract assignment's value (rhs): either a Variable or an IndexAccess
    std::unique_ptr<AbstractCiphertext> ctxt;
    if (auto valueAsVariable = dynamic_cast<Variable *>(&varAssignm.getValue())) {
      // if the value is a Variable: it's sufficient if we clone the corresponding ciphertext
      auto scopedIdentifier = getRootScope().resolveIdentifier(valueAsVariable->getIdentifier());
      ctxt = declaredCiphertexts.at(scopedIdentifier)->clone();
    } else if (auto valueAsIndexAccess = dynamic_cast<IndexAccess *>(&varAssignm.getValue())) {
      // if the value is an IndexAccess we need to clone & rotate the ciphertext accordingly
      try {
        auto valueIdentifier = dynamic_cast<Variable &>(valueAsIndexAccess->getTarget());
        auto idx = dynamic_cast<LiteralInt &>(valueAsIndexAccess->getIndex());
        auto scopedIdentifier = getRootScope().resolveIdentifier(valueIdentifier.getIdentifier());
        ctxt = declaredCiphertexts.at(scopedIdentifier)->rotateRows(idx.getValue());
      } catch (std::bad_cast &) {
        throw std::runtime_error(
            "Nested index accesses in right-hand side of output AST not allowed (e.g., y = __input0__[a[2]]).");
      }
    } else {
      throw std::runtime_error("Right-hand side of output AST is neither a Variable nor IndexAccess "
                               "(e.g., y = __input0__ or y = __input0__[2]).");
    }
    outputValues.emplace_back(identifier, std::move(ctxt));
  } // end: for (auto &assignm : block.getStatements())

  return outputValues;
}

void SpecialRuntimeVisitor::printOutput(AbstractNode &outputAst, std::ostream &targetStream) {
  // retrieve the identifiers mentioned in the output AST, decrypt referred ciphertexts, and print them
  auto outputValues = getOutput(outputAst);
  for (const auto &v : outputValues) targetStream << v.first << ": " << factory.getString(*v.second) << std::endl;
}