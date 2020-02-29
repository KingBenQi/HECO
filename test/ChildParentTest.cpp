#include "gtest/gtest.h"
#include "ArithmeticExpr.h"
#include "Variable.h"
#include "Return.h"
#include "Function.h"
#include "LiteralFloat.h"
#include "LogicalExpr.h"
#include "UnaryExpr.h"
#include "While.h"
#include "Block.h"
#include "Call.h"
#include "CallExternal.h"
#include "If.h"
#include "VarAssignm.h"
#include "VarDecl.h"

class ArithmeticExprFixture : public ::testing::Test {
 protected:
  LiteralInt *left;
  LiteralInt *otherLeft;
  LiteralFloat *right;
  LiteralFloat *otherRight;
  ArithmeticOp opSymb;
  Operator *operatorAdd;

  ArithmeticExprFixture() {
    left = new LiteralInt(3);
    otherLeft = new LiteralInt(42);
    right = new LiteralFloat(2.0);
    otherRight = new LiteralFloat(22.4);
    opSymb = ArithmeticOp::addition;
    operatorAdd = new Operator(opSymb);
  }
};

TEST_F(ArithmeticExprFixture, ArithmeticExprStandardConstructor) {  /* NOLINT */
  auto arithmeticExpr = new ArithmeticExpr(left, opSymb, right);

  // children
  ASSERT_EQ(arithmeticExpr->getChildren().size(), 3);
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(0), left);
  ASSERT_TRUE(reinterpret_cast<Operator *>(arithmeticExpr->getChildAtIndex(1))->equals(opSymb));
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(2), right);

  // parents
  ASSERT_EQ(arithmeticExpr->getParents().size(), 0);
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(arithmeticExpr->getChildAtIndex(0)->hasParent(arithmeticExpr));
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(arithmeticExpr->getChildAtIndex(1)->hasParent(arithmeticExpr));
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(2)->getParents().size(), 1);
  ASSERT_TRUE(arithmeticExpr->getChildAtIndex(2)->hasParent(arithmeticExpr));
}

TEST_F(ArithmeticExprFixture, ArithmeticExprEmptyConstructor) {  /* NOLINT */
  ArithmeticExpr arithmeticExpr;
  ASSERT_EQ(arithmeticExpr.getChildren().size(), 3);
  ASSERT_EQ(arithmeticExpr.countChildrenNonNull(), 0);
  ASSERT_EQ(arithmeticExpr.getParents().size(), 0);
}

TEST_F(ArithmeticExprFixture, ArithmeticExprOperatorOnlyConstructor) {  /* NOLINT */
  auto arithmeticExpr = new ArithmeticExpr(opSymb);

  // children
  ASSERT_EQ(arithmeticExpr->getChildren().size(), 3);
  ASSERT_EQ(arithmeticExpr->countChildrenNonNull(), 1);
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(0), nullptr);
  ASSERT_TRUE(reinterpret_cast<Operator *>(arithmeticExpr->getChildAtIndex(1))->equals(opSymb));
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(2), nullptr);

  // parents
  ASSERT_EQ(arithmeticExpr->getParents().size(), 0);
  ASSERT_EQ(arithmeticExpr->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(arithmeticExpr->getChildAtIndex(1)->hasParent(arithmeticExpr));
}

TEST_F(ArithmeticExprFixture, ArithmeticExprAddChildException_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto arithmeticExpr = new ArithmeticExpr(left, opSymb, right);
  EXPECT_THROW(arithmeticExpr->addChild(new LiteralInt(3), false),
               std::logic_error);
}

TEST_F(ArithmeticExprFixture, ArithmeticExprAddChildException_TooManyChildrenAdded) {  /* NOLINT */
  auto arithmeticExpr = new ArithmeticExpr(left, opSymb, right);
  EXPECT_THROW(arithmeticExpr->addChildren({{left, otherLeft, new Operator(opSymb), right}}, false),
               std::invalid_argument);
}

TEST_F(ArithmeticExprFixture, ArithmeticExprAddChildSuccess) {  /* NOLINT */
  auto arithmeticExpr = new ArithmeticExpr();
  arithmeticExpr->setAttributes(nullptr, operatorAdd, right);
  auto newLeft = new LiteralInt(3);
  arithmeticExpr->addChild(newLeft, true);

  // children
  EXPECT_EQ(arithmeticExpr->getChildren().size(), 3);
  EXPECT_EQ(arithmeticExpr->getLeft(), newLeft);
  EXPECT_EQ(arithmeticExpr->getChildAtIndex(0), newLeft);
  EXPECT_EQ(arithmeticExpr->getOp(), operatorAdd);
  EXPECT_TRUE(reinterpret_cast<Operator *>(arithmeticExpr->getChildAtIndex(1))->equals(opSymb));
  EXPECT_EQ(arithmeticExpr->getRight(), right);
  EXPECT_EQ(arithmeticExpr->getChildAtIndex(2), right);

  // parents
  EXPECT_EQ(newLeft->getParents().size(), 1);
  EXPECT_EQ(newLeft->getParents().front(), arithmeticExpr);
  EXPECT_EQ(operatorAdd->getParents().size(), 1);
  EXPECT_EQ(operatorAdd->getParents().front(), arithmeticExpr);
  EXPECT_EQ(right->getParents().size(), 1);
  EXPECT_EQ(right->getParents().front(), arithmeticExpr);
}

TEST(ChildParentTests, Block) {  /* NOLINT */
  auto blockStatement =
      new Block(new VarAssignm("varX", new LiteralInt(22)));
  ASSERT_EQ(blockStatement->getChildren().size(), 1);
  ASSERT_EQ(blockStatement->getParents().size(), 0);
  ASSERT_TRUE(blockStatement->supportsCircuitMode());
  ASSERT_EQ(blockStatement->getMaxNumberChildren(), -1);
}

TEST(ChildParentTests, Block_addAdditionalChild) {  /* NOLINT */
  auto varDecl = new VarDecl("varX", 22);
  auto blockStatement = new Block(varDecl);
  auto varAssignm = new VarAssignm("varX", new LiteralInt(531));
  blockStatement->addChild(varAssignm);

  ASSERT_TRUE(blockStatement->supportsCircuitMode());
  ASSERT_EQ(blockStatement->getMaxNumberChildren(), -1);

  ASSERT_EQ(blockStatement->getChildren().size(), 2);
  ASSERT_EQ(blockStatement->getParents().size(), 0);

  EXPECT_EQ(blockStatement->getChildAtIndex(0)->getParents().size(), 1);
  EXPECT_EQ(blockStatement->getChildAtIndex(0), varDecl);
  EXPECT_EQ(blockStatement->getChildAtIndex(1)->getParents().size(), 1);
  EXPECT_EQ(blockStatement->getChildAtIndex(1), varAssignm);
}

TEST(ChildParentTests, Call) {  /* NOLINT */
  auto func = new Function("computeSecretX");
  auto funcParam = new FunctionParameter(new Datatype(Types::INT), new LiteralInt(221));
  auto call = new Call({funcParam}, func);

  ASSERT_EQ(call->getChildren().size(), 0);
  ASSERT_EQ(call->getParents().size(), 0);
  ASSERT_FALSE(call->supportsCircuitMode());
  ASSERT_EQ(call->getMaxNumberChildren(), 0);

  // checking children
  ASSERT_EQ(func->getParents().size(), 0);
  ASSERT_EQ(funcParam->getParents().size(), 0);
}

TEST(ChildParentTests, CallExternal) {  /* NOLINT */
  auto callExternal = new CallExternal("computeSecretKeys");

  // using AbstractExpr
  ASSERT_EQ(callExternal->AbstractExpr::getChildren().size(), 0);
  ASSERT_EQ(callExternal->AbstractExpr::getParents().size(), 0);
  ASSERT_FALSE(callExternal->AbstractExpr::supportsCircuitMode());
  ASSERT_EQ(callExternal->AbstractExpr::getMaxNumberChildren(), 0);

  // using AbstractStatement
  ASSERT_EQ(callExternal->AbstractStatement::getChildren().size(), 0);
  ASSERT_EQ(callExternal->AbstractStatement::getParents().size(), 0);
  ASSERT_FALSE(callExternal->AbstractStatement::supportsCircuitMode());
  ASSERT_EQ(callExternal->AbstractStatement::getMaxNumberChildren(), 0);
}

class FunctionFixture : public ::testing::Test {
 protected:
  Function *funcComputeX;
  AbstractStatement *returnStatement;

  FunctionFixture() {
    funcComputeX = new Function("computeX");
    returnStatement = new Return(new LiteralBool(true));
  }
};

TEST_F(FunctionFixture, FunctionAddStatement) {  /* NOLINT */
  funcComputeX->addStatement(returnStatement);
  ASSERT_EQ(funcComputeX->getChildren().size(), 2);
  ASSERT_EQ(returnStatement->getParents().size(), 1);
}

TEST_F(FunctionFixture, FunctionSupportedInCircuitMode) {  /* NOLINT */
  // Function is circuit-compatible, i.e., supports use of child/parent relationship
  ASSERT_EQ(funcComputeX->getChildren().size(), 2);
  ASSERT_EQ(returnStatement->getParents().size(), 0);
  ASSERT_TRUE(funcComputeX->supportsCircuitMode());
  ASSERT_EQ(funcComputeX->getMaxNumberChildren(), 2);
}

class FunctionParameterFixture : public ::testing::Test {
 protected:
  Datatype *datatype;
  Datatype *datatype2;
  Types datatypeEnum;
  std::string datatypeAsString;
  AbstractExpr *variableThreshold;
  AbstractExpr *variableSecret;

  FunctionParameterFixture() {
    datatypeEnum = Types::INT;
    datatypeAsString = Datatype::enumToString(datatypeEnum);
    datatype = new Datatype(datatypeEnum);
    datatype2 = new Datatype(Types::FLOAT);
    variableThreshold = new Variable("threshold");
    variableSecret = new Variable("secretNumber");
  }
};

TEST_F(FunctionParameterFixture, FunctionParameterStandardConstructor) {  /* NOLINT */
  auto functionParameter = new FunctionParameter(datatypeAsString, variableThreshold);

  // children
  ASSERT_EQ(functionParameter->getChildren().size(), 2);
  ASSERT_EQ(functionParameter->getChildAtIndex(0)->castTo<Datatype>()->getType(), datatypeEnum);
  ASSERT_EQ(functionParameter->getChildAtIndex(1), variableThreshold);

  // parents
  ASSERT_EQ(functionParameter->getParents().size(), 0);
  ASSERT_EQ(functionParameter->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(functionParameter->getChildAtIndex(0)->hasParent(functionParameter));
  ASSERT_EQ(functionParameter->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(functionParameter->getChildAtIndex(1)->hasParent(functionParameter));
}

TEST_F(FunctionParameterFixture, FunctionParameterAddChildExceptionDatatypeConstructor) {  /* NOLINT */
  auto functionParameter = new FunctionParameter(datatype, variableThreshold);

  // children
  ASSERT_EQ(functionParameter->getChildren().size(), 2);
  ASSERT_EQ(functionParameter->getChildAtIndex(0), datatype);
  ASSERT_EQ(functionParameter->getChildAtIndex(1), variableThreshold);

  // parents
  ASSERT_EQ(functionParameter->getParents().size(), 0);
  ASSERT_EQ(functionParameter->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(functionParameter->getChildAtIndex(0)->hasParent(functionParameter));
  ASSERT_EQ(functionParameter->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(functionParameter->getChildAtIndex(1)->hasParent(functionParameter));
}

TEST_F(FunctionParameterFixture, FunctionParameterAddChildException_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto functionParameter = new FunctionParameter(datatype, variableThreshold);
  EXPECT_THROW(functionParameter->addChild(variableSecret, false), std::logic_error);
}

TEST_F(FunctionParameterFixture, FunctionParameterAddChildException_TooManyChildrenAdded) {  /* NOLINT */
  auto functionParameter = new FunctionParameter(datatype, variableThreshold);
  EXPECT_THROW(functionParameter->addChildren({{datatype, variableSecret, variableThreshold}}, false),
               std::invalid_argument);
}

TEST_F(FunctionParameterFixture, FunctionParameter_AddChildSuccess) {  /* NOLINT */
  auto functionParameter = new FunctionParameter(datatype, variableThreshold);

  functionParameter->removeChild(variableThreshold, false);
  functionParameter->addChild(variableSecret, true);

  // children
  EXPECT_EQ(functionParameter->getChildren().size(), 2);
  EXPECT_EQ(functionParameter->getValue(), variableSecret);
  EXPECT_EQ(functionParameter->getChildAtIndex(1), variableSecret);

  // parents
  EXPECT_EQ(functionParameter->getParents().size(), 0);
  EXPECT_EQ(variableSecret->getParents().size(), 1);
  EXPECT_EQ(variableSecret->getParents().front(), functionParameter);
  EXPECT_TRUE(variableSecret->hasParent(functionParameter));

  functionParameter->removeChild(datatype, false);
  functionParameter->addChild(datatype2, true);

  // children
  EXPECT_EQ(functionParameter->getChildren().size(), 2);
  EXPECT_EQ(functionParameter->getDatatype(), datatype2);
  EXPECT_EQ(functionParameter->getChildAtIndex(0), datatype2);

  // parents
  EXPECT_EQ(functionParameter->getParents().size(), 0);
  EXPECT_EQ(datatype2->getParents().size(), 1);
  EXPECT_EQ(datatype2->getParents().front(), functionParameter);
  EXPECT_TRUE(datatype2->hasParent(functionParameter));
}

class IfStmtFixture : public ::testing::Test {
 protected:
  AbstractExpr *condition;
  AbstractStatement *thenBranch, *elseBranch;
  IfStmtFixture() {
    condition = new LogicalExpr(new LiteralInt(33), LogCompOp::greater, new CallExternal("computeX"));
    thenBranch = new Block(new VarAssignm("a", new LiteralInt(22)));
    elseBranch = new Block(new VarAssignm("a", new LiteralInt(175)));
  }
};

TEST_F(IfStmtFixture, IfStmtThenOnlyConstructor) {  /* NOLINT */
  auto ifStmt = new If(condition, thenBranch);

  // children
  ASSERT_EQ(ifStmt->getChildren().size(), 3);
  ASSERT_EQ(ifStmt->getChildAtIndex(0), condition);
  ASSERT_EQ(ifStmt->getChildAtIndex(1), thenBranch);

  // parents
  ASSERT_EQ(ifStmt->getParents().size(), 0);
  ASSERT_EQ(ifStmt->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(ifStmt->getChildAtIndex(0)->hasParent(ifStmt));
  ASSERT_EQ(ifStmt->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(ifStmt->getChildAtIndex(1)->hasParent(ifStmt));
}

TEST_F(IfStmtFixture, IfStmtThenAndElseConstructor) {  /* NOLINT */
  auto ifStmt = new If(condition, thenBranch, elseBranch);

  // children
  ASSERT_EQ(ifStmt->getChildren().size(), 3);
  ASSERT_EQ(ifStmt->getChildAtIndex(0), condition);
  ASSERT_EQ(ifStmt->getChildAtIndex(1), thenBranch);
  ASSERT_EQ(ifStmt->getChildAtIndex(2), elseBranch);

  // parents
  ASSERT_EQ(ifStmt->getParents().size(), 0);
  ASSERT_EQ(ifStmt->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(ifStmt->getChildAtIndex(0)->hasParent(ifStmt));
  ASSERT_EQ(ifStmt->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(ifStmt->getChildAtIndex(1)->hasParent(ifStmt));
  ASSERT_EQ(ifStmt->getChildAtIndex(2)->getParents().size(), 1);
  ASSERT_TRUE(ifStmt->getChildAtIndex(2)->hasParent(ifStmt));
}

TEST_F(IfStmtFixture, IfStmtAddChildException_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto ifStmt = new If(condition, thenBranch, elseBranch);
  EXPECT_THROW(ifStmt->addChild(new VarAssignm("a", new LiteralInt(22222)), false),
               std::logic_error);
}

TEST_F(IfStmtFixture, IfStmtAddChildException_TooManyChildrenAdded) {  /* NOLINT */
  auto ifStmt = new If(condition, thenBranch);
  auto newElseBranch = new Block(new VarAssignm("a", new LiteralInt(1024)));
  EXPECT_THROW(ifStmt->addChildren({{elseBranch, newElseBranch}}, false),
               std::invalid_argument);
}

TEST_F(IfStmtFixture, IfStmtAddChildSuccess) {  /* NOLINT */
  auto ifStmt = new If(condition, thenBranch);
  auto newElseBranch = new Block(new VarAssignm("a", new LiteralInt(1024)));
  ifStmt->addChild(newElseBranch, true);

  // children
  EXPECT_EQ(ifStmt->getChildren().size(), 3);
  EXPECT_EQ(ifStmt->getCondition(), condition);
  EXPECT_EQ(ifStmt->getChildAtIndex(0), condition);
  EXPECT_EQ(ifStmt->getThenBranch(), thenBranch);
  EXPECT_EQ(ifStmt->getChildAtIndex(1), thenBranch);
  EXPECT_EQ(ifStmt->getElseBranch(), newElseBranch);
  EXPECT_EQ(ifStmt->getChildAtIndex(2), newElseBranch);

  // parents
  EXPECT_EQ(condition->getParents().size(), 1);
  EXPECT_EQ(condition->getParents().front(), ifStmt);
  EXPECT_EQ(thenBranch->getParents().size(), 1);
  EXPECT_EQ(thenBranch->getParents().front(), ifStmt);
  EXPECT_EQ(newElseBranch->getParents().size(), 1);
  EXPECT_EQ(newElseBranch->getParents().front(), ifStmt);
}

TEST(ChildParentTests, LiteralBoolHasNoChildrenOrParents) {  /* NOLINT */
  // Literals should never have any children
  LiteralBool literalBool(true);
  ASSERT_TRUE(literalBool.getChildren().empty());
  ASSERT_TRUE(literalBool.getParents().empty());

  literalBool.setValue(false);
  ASSERT_TRUE(literalBool.getChildren().empty());
  ASSERT_TRUE(literalBool.getParents().empty());
}

TEST(ChildParentTests, LiteralFloatHasNoChildrenOrParents) {  /* NOLINT */
  // Literals should never have any children
  LiteralFloat literalFloat(true);
  ASSERT_TRUE(literalFloat.getChildren().empty());
  ASSERT_TRUE(literalFloat.getParents().empty());

  literalFloat.setValue(false);
  ASSERT_TRUE(literalFloat.getChildren().empty());
  ASSERT_TRUE(literalFloat.getParents().empty());
}

TEST(ChildParentTests, LiteralIntHasNoChildrenOrParents) {  /* NOLINT */
  // Literals should never have any children
  LiteralInt literalInt(33);
  ASSERT_TRUE(literalInt.getChildren().empty());
  ASSERT_TRUE(literalInt.getParents().empty());

  literalInt.setValue(111);
  ASSERT_TRUE(literalInt.getChildren().empty());
  ASSERT_TRUE(literalInt.getParents().empty());
}

TEST(ChildParentTests, LiteralStringHasNoChildrenOrParents) {  /* NOLINT */
  // Literals should never have any children
  LiteralString literalString("alpha");
  ASSERT_TRUE(literalString.getChildren().empty());
  ASSERT_TRUE(literalString.getParents().empty());

  literalString.setValue("beta");
  ASSERT_TRUE(literalString.getChildren().empty());
  ASSERT_TRUE(literalString.getParents().empty());
}

class LogicalExprFixture : public ::testing::Test {
 protected:
  LiteralInt *literalInt;
  LiteralInt *literalIntAnother;
  LiteralBool *literalBool;
  LogCompOp opSymb;
  Operator *operatorGreaterEqual;

  LogicalExprFixture() {
    literalInt = new LiteralInt(24);
    literalIntAnother = new LiteralInt(6245);
    literalBool = new LiteralBool(true);
    opSymb = LogCompOp::greaterEqual;
    operatorGreaterEqual = new Operator(opSymb);
  }
};

TEST_F(LogicalExprFixture, LogicalExprStandardConstructor) {  /* NOLINT */
  auto logicalExpr = new LogicalExpr(literalInt, opSymb, literalIntAnother);

  // children
  ASSERT_EQ(logicalExpr->getChildren().size(), 3);
  ASSERT_EQ(logicalExpr->getChildAtIndex(0), literalInt);
  ASSERT_TRUE(reinterpret_cast<Operator *>(logicalExpr->getChildAtIndex(1))->equals(opSymb));
  ASSERT_EQ(logicalExpr->getChildAtIndex(2), literalIntAnother);

  // parents
  ASSERT_EQ(logicalExpr->getParents().size(), 0);
  ASSERT_EQ(logicalExpr->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(logicalExpr->getChildAtIndex(0)->hasParent(logicalExpr));
  ASSERT_EQ(logicalExpr->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(logicalExpr->getChildAtIndex(1)->hasParent(logicalExpr));
  ASSERT_EQ(logicalExpr->getChildAtIndex(2)->getParents().size(), 1);
  ASSERT_TRUE(logicalExpr->getChildAtIndex(2)->hasParent(logicalExpr));
}

TEST_F(LogicalExprFixture, LogicalExprEmptyConstructor) {  /* NOLINT */
  ArithmeticExpr logicalExpr;
  ASSERT_EQ(logicalExpr.getChildren().size(), 3);
  ASSERT_EQ(logicalExpr.countChildrenNonNull(), 0);
  ASSERT_EQ(logicalExpr.getParents().size(), 0);
}

TEST_F(LogicalExprFixture, LogicalExprOperatorOnlyConstructor) {  /* NOLINT */
  auto logicalExpr = new LogicalExpr(opSymb);

  // children
  ASSERT_EQ(logicalExpr->getChildren().size(), 3);
  ASSERT_EQ(logicalExpr->countChildrenNonNull(), 1);
  ASSERT_EQ(logicalExpr->getChildAtIndex(0), nullptr);
  ASSERT_TRUE(reinterpret_cast<Operator *>(logicalExpr->getChildAtIndex(1))->equals(opSymb));
  ASSERT_EQ(logicalExpr->getChildAtIndex(2), nullptr);

  // parents
  ASSERT_EQ(logicalExpr->getParents().size(), 0);
  ASSERT_EQ(logicalExpr->getChildAtIndex(1)->getParents().size(), 1);
  ASSERT_TRUE(logicalExpr->getChildAtIndex(1)->hasParent(logicalExpr));
}

TEST_F(LogicalExprFixture, LogicalExprAddChildException_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto logicalExpr = new LogicalExpr(literalInt, opSymb, literalIntAnother);
  EXPECT_THROW(logicalExpr->addChild(new LiteralInt(3), false),
               std::logic_error);
}

TEST_F(LogicalExprFixture, LogicalExprAddChildException_TooManyChildrenAdded) {  /* NOLINT */
  auto logicalExpr = new LogicalExpr(literalInt, opSymb, literalIntAnother);
  EXPECT_THROW(logicalExpr->addChildren({{literalInt, literalIntAnother, new Operator(opSymb), literalBool}}, false),
               std::invalid_argument);
}

TEST_F(LogicalExprFixture, LogicalExprAddChildSuccess) {  /* NOLINT */
  auto logicalExpr = new LogicalExpr();
  logicalExpr->setAttributes(nullptr, operatorGreaterEqual, literalBool);
  logicalExpr->addChild(literalIntAnother, true);

  // children
  EXPECT_EQ(logicalExpr->getChildren().size(), 3);
  EXPECT_EQ(logicalExpr->getLeft(), literalIntAnother);
  EXPECT_EQ(logicalExpr->getChildAtIndex(0), literalIntAnother);
  EXPECT_EQ(logicalExpr->getOp(), operatorGreaterEqual);
  EXPECT_TRUE(reinterpret_cast<Operator *>(logicalExpr->getChildAtIndex(1))->equals(opSymb));
  EXPECT_EQ(logicalExpr->getRight(), literalBool);
  EXPECT_EQ(logicalExpr->getChildAtIndex(2), literalBool);

  // parents
  EXPECT_EQ(literalIntAnother->getParents().size(), 1);
  EXPECT_EQ(literalIntAnother->getParents().front(), logicalExpr);
  EXPECT_EQ(operatorGreaterEqual->getParents().size(), 1);
  EXPECT_EQ(operatorGreaterEqual->getParents().front(), logicalExpr);
  EXPECT_EQ(literalBool->getParents().size(), 1);
  EXPECT_EQ(literalBool->getParents().front(), logicalExpr);
}

TEST(ChildParentTests, OperatorHasNoChildrenOrParents) {  /* NOLINT */
  Operator op(LogCompOp::greaterEqual);
  ASSERT_TRUE(op.getChildren().empty());
  ASSERT_TRUE(op.getParents().empty());
}

class ReturnStatementFixture : public ::testing::Test {
 protected:
  AbstractExpr *abstractExpr;
  AbstractExpr *abstractExprOther;

  ReturnStatementFixture() {
    abstractExpr = new LiteralInt(22);
    abstractExprOther = new LiteralBool(true);
  }
};

TEST_F(ReturnStatementFixture, ReturnStatementStandardConstructor) {  /* NOLINT */
  auto returnStatement = new Return(abstractExpr);

  // children
  ASSERT_EQ(returnStatement->getChildren().size(), 1);
  ASSERT_EQ(returnStatement->getChildren().front(), abstractExpr);

  // parent
  ASSERT_EQ(returnStatement->getParents().size(), 0);
  ASSERT_EQ(returnStatement->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(returnStatement->getChildAtIndex(0)->hasParent(returnStatement));
}

TEST_F(ReturnStatementFixture, ReturnStatementEmptyConstructor) {  /* NOLINT */
  Return returnStatement;
  ASSERT_EQ(returnStatement.getChildren().size(), 0);
  ASSERT_EQ(returnStatement.countChildrenNonNull(), 0);
  ASSERT_EQ(returnStatement.getParents().size(), 0);
}

TEST_F(ReturnStatementFixture, ReturnStatementAddSecondChild) {  /* NOLINT */
  auto returnStatement = new Return(abstractExpr);
  returnStatement->addChild(abstractExprOther, false);
  EXPECT_EQ(returnStatement->getChildren().size(), 2);
  EXPECT_EQ(returnStatement->getChildrenNonNull().size(), 2);
}

TEST_F(ReturnStatementFixture, ReturnStatementAddChildSuccess) {  /* NOLINT */
  auto returnStatement = new Return();
  returnStatement->addChild(abstractExprOther, true);
  EXPECT_EQ(returnStatement->getReturnExpressions().front(), abstractExprOther);
  EXPECT_EQ(returnStatement->getChildren().size(), 1);
  EXPECT_EQ(returnStatement->getChildren().front(), abstractExprOther);
  EXPECT_EQ(abstractExprOther->getParents().size(), 1);
  EXPECT_EQ(abstractExprOther->getParents().front(), returnStatement);
}

class UnaryExprFixture : public ::testing::Test {
 protected:
  UnaryOp opSymbNegation;
  LiteralBool *literalBoolTrue;

  UnaryExprFixture() {
    opSymbNegation = UnaryOp::negation;
    new Operator(opSymbNegation);
    literalBoolTrue = new LiteralBool(true);
  }
};

TEST_F(UnaryExprFixture, UnaryExprStandardConstructor) {  /* NOLINT */
  auto unaryExpr = new UnaryExpr(opSymbNegation, literalBoolTrue);

  // children
  ASSERT_EQ(unaryExpr->getChildren().size(), 2);
  ASSERT_TRUE(reinterpret_cast<Operator *>(unaryExpr->getChildAtIndex(0))->equals(opSymbNegation));
  ASSERT_EQ(unaryExpr->getChildAtIndex(1), literalBoolTrue);

  // parents
  ASSERT_EQ(unaryExpr->getParents().size(), 0);
  ASSERT_TRUE(unaryExpr->getChildAtIndex(0)->hasParent(unaryExpr));
  ASSERT_TRUE(unaryExpr->getChildAtIndex(1)->hasParent(unaryExpr));
}

TEST_F(UnaryExprFixture, UnaryExprAddChildException_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto unaryExpr = new UnaryExpr(opSymbNegation, literalBoolTrue);
  EXPECT_THROW(unaryExpr->addChild(new Operator(UnaryOp::negation), false), std::logic_error);
}

TEST_F(UnaryExprFixture, UnaryExprAddChildException_TooManyChildrenAdded) {  /* NOLINT */
  auto unaryExpr = new UnaryExpr(opSymbNegation, literalBoolTrue);
  EXPECT_THROW(unaryExpr->addChildren({new Operator(UnaryOp::negation), new LiteralBool(false)}, false),
               std::logic_error);
}

TEST_F(UnaryExprFixture, UnaryExprtion_AddChildSuccess) {  /* NOLINT */
  auto unaryExpr = new UnaryExpr(opSymbNegation, literalBoolTrue);

  unaryExpr->removeChild(unaryExpr->getOp(), false);
  auto newOperator = new Operator(UnaryOp::negation);
  unaryExpr->addChild(newOperator, true);

  // children
  EXPECT_EQ(unaryExpr->getChildren().size(), 2);
  EXPECT_EQ(*unaryExpr->getOp(), *newOperator);
  EXPECT_TRUE(reinterpret_cast<Operator *>(unaryExpr->getChildAtIndex(0))->equals(newOperator->getOperatorSymbol()));

  // parents
  EXPECT_EQ(unaryExpr->getParents().size(), 0);
  EXPECT_EQ(unaryExpr->getChildAtIndex(0)->getParents().size(), 1);
  EXPECT_TRUE(unaryExpr->getChildAtIndex(0)->hasParent(unaryExpr));
}

class VarAssignmFixture : public ::testing::Test {
 protected:
  LiteralInt *literalInt222;
  std::string variableIdentifier;

  VarAssignmFixture() {
    literalInt222 = new LiteralInt(222);
    variableIdentifier = "secretX";
  }
};

TEST_F(VarAssignmFixture, VarAssignmStandardConstructor) {  /* NOLINT */
  auto varAssignm = new VarAssignm(variableIdentifier, literalInt222);

  // children
  ASSERT_EQ(varAssignm->getChildren().size(), 1);
  ASSERT_EQ(varAssignm->getChildAtIndex(0), literalInt222);

  // parents
  ASSERT_EQ(varAssignm->getParents().size(), 0);
  ASSERT_EQ(varAssignm->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(varAssignm->getChildAtIndex(0)->hasParent(varAssignm));
}

TEST_F(VarAssignmFixture, VarAssignm_NoEmptyChildSpotAvailable) {  /* NOLINT */
  auto varAssignm = new VarAssignm(variableIdentifier, literalInt222);
  EXPECT_THROW(varAssignm->addChild(new LiteralBool(true), false), std::logic_error);
}

TEST_F(VarAssignmFixture, VarAssignm_TooManyChildrenAdded) {  /* NOLINT */
  auto varAssignm = new VarAssignm(variableIdentifier, literalInt222);
  EXPECT_THROW(varAssignm->addChildren({new LiteralBool(true), new LiteralInt(5343)}, false), std::invalid_argument);
}

TEST_F(VarAssignmFixture, VarAssignmAddChildSuccess) {  /* NOLINT */
  auto varAssignm = new VarAssignm(variableIdentifier, literalInt222);

  varAssignm->removeChildren();
  auto newChild = new LiteralBool(false);
  varAssignm->addChild(newChild, true);

  // children
  ASSERT_EQ(varAssignm->getChildren().size(), 1);
  ASSERT_EQ(varAssignm->getChildAtIndex(0), newChild);

  // parents
  ASSERT_EQ(varAssignm->getParents().size(), 0);
  ASSERT_EQ(varAssignm->getChildAtIndex(0)->getParents().size(), 1);
  ASSERT_TRUE(varAssignm->getChildAtIndex(0)->hasParent(varAssignm));
}

class VarDeclFixture : public ::testing::Test {
 protected:
  int integerValue;
  float floatValue;
  bool boolValue;
  std::string stringValue;
  LiteralInt *literalInt;
  std::string variableIdentifier;
  Types datatypeInt;

  VarDeclFixture() {
    literalInt = new LiteralInt(integerValue);
    integerValue = 343224;
    variableIdentifier = "maxValue";
    floatValue = 2.42f;
    boolValue = false;
    stringValue = "Determines the maximum allowed value";
    datatypeInt = Types::INT;
  }

  static void checkExpected(VarDecl *varDeclaration, Datatype *expectedDatatype, AbstractExpr *expectedValue) {
    // children
    ASSERT_EQ(varDeclaration->getChildren().size(), 2);
    ASSERT_EQ(reinterpret_cast<Datatype *>(varDeclaration->getChildAtIndex(0)), expectedDatatype);
    ASSERT_EQ(varDeclaration->getChildAtIndex(1), expectedValue);

    // parents
    ASSERT_EQ(varDeclaration->getParents().size(), 0);
    ASSERT_EQ(varDeclaration->getChildAtIndex(0)->getParents().size(), 1);
    ASSERT_TRUE(varDeclaration->getChildAtIndex(0)->hasParent(varDeclaration));
    ASSERT_EQ(varDeclaration->getChildAtIndex(1)->getParents().size(), 1);
    ASSERT_TRUE(varDeclaration->getChildAtIndex(1)->hasParent(varDeclaration));
  }
};

TEST_F(VarDeclFixture, VarDeclStandardConstructor) {  /* NOLINT */
  auto variableDeclaration = new VarDecl(variableIdentifier, datatypeInt, literalInt);
  ASSERT_EQ(reinterpret_cast<Datatype *>(variableDeclaration->getDatatype())->toString(false),
            Datatype(datatypeInt).toString(false));
  checkExpected(variableDeclaration, variableDeclaration->getDatatype(), literalInt);
}

TEST_F(VarDeclFixture, VarDeclIntConstructor) {  /* NOLINT */
  auto variableDeclaration = new VarDecl(variableIdentifier, integerValue);
  ASSERT_EQ(reinterpret_cast<LiteralInt *>(variableDeclaration->getInitializer())->getValue(), integerValue);
  ASSERT_EQ(reinterpret_cast<Datatype *>(variableDeclaration->getDatatype())->toString(false),
            Datatype(Types::INT).toString(false));
  checkExpected(variableDeclaration, variableDeclaration->getDatatype(), variableDeclaration->getInitializer());
}

TEST_F(VarDeclFixture, VarDeclBoolConstructor) {  /* NOLINT */
  auto variableDeclaration = new VarDecl(variableIdentifier, boolValue);
  ASSERT_EQ(reinterpret_cast<LiteralBool *>(variableDeclaration->getInitializer())->getValue(), boolValue);
  ASSERT_EQ(reinterpret_cast<Datatype *>(variableDeclaration->getDatatype())->toString(false),
            Datatype(Types::BOOL).toString(false));
  checkExpected(variableDeclaration, variableDeclaration->getDatatype(), variableDeclaration->getInitializer());
}

TEST_F(VarDeclFixture, VarDeclFloatConstructor) {  /* NOLINT */
  auto variableDeclaration = new VarDecl(variableIdentifier, floatValue);
  ASSERT_EQ(reinterpret_cast<LiteralFloat *>(variableDeclaration->getInitializer())->getValue(), floatValue);
  ASSERT_EQ(reinterpret_cast<Datatype *>(variableDeclaration->getDatatype())->toString(false),
            Datatype(Types::FLOAT).toString(false));
  checkExpected(variableDeclaration, variableDeclaration->getDatatype(), variableDeclaration->getInitializer());
}

TEST_F(VarDeclFixture, VarDeclStringConstructor) {  /* NOLINT */
  auto variableDeclaration = new VarDecl(variableIdentifier, stringValue);
  ASSERT_EQ(reinterpret_cast<LiteralString *>(variableDeclaration->getInitializer())->getValue(), stringValue);
  ASSERT_EQ(reinterpret_cast<Datatype *>(variableDeclaration->getDatatype())->toString(false),
            Datatype(Types::STRING).toString(false));
  checkExpected(variableDeclaration, variableDeclaration->getDatatype(), variableDeclaration->getInitializer());
}

TEST(ChildParentTests, Variable) {  /* NOLINT */
  Variable variable("myInt");
  ASSERT_TRUE(variable.getChildren().empty());
  ASSERT_TRUE(variable.getParents().empty());
}

TEST(ChildParentTests, While) {  /* NOLINT */
  auto whileStatement =
      new While(new LogicalExpr(new LiteralInt(32), LogCompOp::greaterEqual, new Variable("a")), new Block());
  ASSERT_EQ(whileStatement->getChildren().size(), 0);
  ASSERT_EQ(whileStatement->getParents().size(), 0);
  ASSERT_FALSE(whileStatement->supportsCircuitMode());
  ASSERT_EQ(whileStatement->getMaxNumberChildren(), 0);
}
