#include <queue>
#include <iostream>
#include "ast_opt/ast/AbstractTarget.h"
#include "ast_opt/ast/Assignment.h"
#include "ast_opt/ast/BinaryExpression.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/ExpressionList.h"
#include "ast_opt/ast/IndexAccess.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/OperatorExpression.h"
#include "ast_opt/ast/UnaryExpression.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/visitor/ExpressionBatcher.h"

////////////////////////////////////////////
////         ExpressionBatching         ////
////////////////////////////////////////////

class ComputationOperator {
 private:
  Operator op;
  bool batched = false;
 public:
  ComputationOperator(const Operator &op, bool batched = false) : op(op), batched(batched) {}

};

/// Invariant (not enforced by class): if computationOperator::batched == true, then children.size() must be 2
class TreeNode {
 private:
  TreeNode *parent = nullptr;
  std::vector<std::unique_ptr<TreeNode>> children;

 public:
  virtual ~TreeNode() = default;
  TreeNode() = default;
  TreeNode(TreeNode *parent, std::vector<std::unique_ptr<TreeNode>> &&children) :
      parent(parent), children(std::move(children)) {}
  TreeNode(const TreeNode &other) = delete;
  TreeNode(TreeNode &&other) noexcept = default;
  TreeNode &operator=(const TreeNode &other) = delete;
  TreeNode &operator=(TreeNode &&other) = default;

  void addChild(std::unique_ptr<TreeNode> &&newChild) {
    newChild->parent = this;
    for (auto &p : children) {
      if (p==nullptr) {
        p = std::move(newChild);
        return;
      }
    }
    throw std::runtime_error("Did not find empty slot to insert child into.");
  }
  TreeNode *getParent() { return parent; }
  std::vector<std::unique_ptr<TreeNode>> &getChildren() { return children; }
  [[nodiscard]] size_t getExpectedNumberOfChildren() const { return children.size(); }
};

class ComputationNode : public TreeNode {
 private:
  ComputationOperator
      computationOperator = ComputationOperator(Operator(ArithmeticOp::FHE_ADDITION)); //Dummy since no () ctor
 public:
  ~ComputationNode() override = default;
  ComputationNode() = default;
  ComputationNode(TreeNode *parent,
                  std::vector<std::unique_ptr<TreeNode>> &&children,
                  ComputationOperator computationOperator) : TreeNode(parent, std::move(children)),
                                                             computationOperator(computationOperator) {}
  ComputationNode(const ComputationNode &other) = delete;
  ComputationNode(ComputationNode &&other) noexcept = default;
  ComputationNode &operator=(const ComputationNode &other) = delete;
  ComputationNode &operator=(ComputationNode &&other) = default;

  ComputationOperator &getComputationOperator() { return computationOperator; }
};

class ValueNode : public TreeNode {
 private:
  AbstractNode *value;
 public:
  ~ValueNode() override = default;
  ValueNode() = default;
  ValueNode(TreeNode *parent, std::vector<std::unique_ptr<TreeNode>> &&children, AbstractNode &value) :
      TreeNode(parent, std::move(children)),
      value(&value) {}
  ValueNode(const ValueNode &other) = delete;
  ValueNode(ValueNode &&other) noexcept = default;
  ValueNode &operator=(const ValueNode &other) = delete;
  ValueNode &operator=(ValueNode &&other) = default;

};
bool operator==(const std::unique_ptr<TreeNode> &sp, const TreeNode *const p) { return sp.get()==p; }

ComplexValue SpecialExpressionBatcher::batchExpression(AbstractExpression &expr, BatchingConstraint) {

  /// Holds nodes of the current level
  std::deque<std::reference_wrapper<AbstractNode>> currentLevelNodes({expr});

  std::unique_ptr<TreeNode> computationTree = nullptr;
  TreeNode *curTreeNode = nullptr;

  /// Each iteration of the loop corresponds to handling one level of the expression tree
  while (!currentLevelNodes.empty()) {

    // Get the next node to process
    auto &curAbstractNode = currentLevelNodes.front().get();
    currentLevelNodes.pop_front();

    // Handle the node itself
    std::unique_ptr<TreeNode> treeNode = nullptr;

    if (auto binaryExpression = dynamic_cast<BinaryExpression *>(&curAbstractNode)) {
      std::vector<std::unique_ptr<TreeNode>> children;
      children.emplace_back(nullptr);
      children.emplace_back(nullptr);
      treeNode = std::make_unique<ComputationNode>(
          nullptr,
          std::move(children),
          ComputationOperator(binaryExpression->getOperator(), true));
    } else if (auto operatorExpression = dynamic_cast<OperatorExpression *>(&curAbstractNode)) {
      std::vector<std::unique_ptr<TreeNode>> children(operatorExpression->countChildren());
      //TODO: If not a power-of-two, fill the rest of the children with the neutral element for the operator
      treeNode = std::make_unique<ComputationNode>(
          nullptr,
          std::move(children),
          ComputationOperator(operatorExpression->getOperator(), true));
    } else if (auto variable = dynamic_cast<Variable *>(&curAbstractNode)) {
      //TODO: Handle batchingConstraints if they exist
      std::vector<std::unique_ptr<TreeNode>> children;
      treeNode = std::make_unique<ValueNode>(nullptr, std::move(children), *variable);
    } else if (auto indexAccess = dynamic_cast<IndexAccess *>(&curAbstractNode)) {
      //TODO: Handle batchingConstraints
      std::vector<std::unique_ptr<TreeNode>> children;
      treeNode = std::make_unique<ValueNode>(nullptr, std::move(children), *indexAccess);
    } else if (isLiteral(curAbstractNode)) {
      std::vector<std::unique_ptr<TreeNode>> children;
      treeNode = std::make_unique<ValueNode>(nullptr, std::move(children), curAbstractNode);
    } else if (auto expressionList = dynamic_cast<ExpressionList *>(&curAbstractNode)) {
      //TODO: Implement ExpressionList batching
      throw std::runtime_error("ExpressionLists currently not supported in expression batching.");
    } else if (auto unaryExpression = dynamic_cast<UnaryExpression *>(&curAbstractNode)) {
      //TODO: Implement UnaryExpression batching
      throw std::runtime_error("Unary Expressions currently not supported in expression batching.");
    } else {
      throw std::runtime_error("Unsupported type of AST node: " + std::string(typeid(curAbstractNode).name()));
    }

    // TODO: Actually handle the AST node and create computationNode from it


    // Update Tree
    if (!computationTree) {
      computationTree = std::move(treeNode);
      curTreeNode = computationTree.get();
    } else if (curTreeNode->getChildren().size() > curTreeNode->getExpectedNumberOfChildren()) {
      curTreeNode->addChild(std::move(treeNode));
    } else {
      std::vector<std::unique_ptr<TreeNode>> &v = curTreeNode->getParent()->getChildren();
      auto it = std::find(v.begin(), v.end(), curTreeNode);

      while (curTreeNode->getChildren().size() >= curTreeNode->getExpectedNumberOfChildren()) {
        // Advance to next element
        if (it==v.end()) {
          throw std::runtime_error("Cannot add Node to ComputationTree since tree ended unexpectedly.");
        } else {
          ++it;
          curTreeNode = it->get();
        }
      }

      curTreeNode->addChild(std::move(treeNode));
    }

    // enqueue children of curNode to process next
    std::deque<std::reference_wrapper<AbstractNode>> nextLevelNodes;
    nextLevelNodes.insert(nextLevelNodes.end(), curAbstractNode.begin(), curAbstractNode.end());

    // Are we at the end of a level?
    if (currentLevelNodes.empty()) {
      currentLevelNodes = std::move(nextLevelNodes);
      //TODO: Update curTreeNode somehow?
    }

  } // END WHILE

  //TODO: Implement conversion of Tree to ComplexValue!
  return ComplexValue(expr);
  /// OLD CODE:

//  // Determine Number of children?
//  if (numChildrenPerNode==-1) {
//    numChildrenPerNode = (int) curNode.countChildren();
//  } else if (numChildrenPerNode!=curNode.countChildren()) {
//    // a subtree is batching incompatible if the children have a different number of operands
//    //TODO: Implement non-symmetric batching
//    throw std::runtime_error("Batching of expressions with different number of children is not yet supported.");
//  }
//
//  if (unprocessedNodes.empty()) {
//
//    // Check that all children are batching compatible to each other
//    AbstractNode &baseNode = childNodes.front().get();
//    for (auto nodeIt = std::next(childNodes.begin()); nodeIt!=childNodes.end(); ++nodeIt) {
//      // check batching compatibility
//      // TODO: Change this to actually construct the new expression as we go!
//      if (!isBatchingCompatible(baseNode, *nodeIt)) {
//        // if we detected a batching incompatibility, we can stop any further testing
//        // TODO: Implement Support for this case!
//        throw std::runtime_error("Rewriting of batching-incompatible expressions is not yet supported.");
//      } else if (isTransparentNode(*nodeIt)) {
//        // as we allow max. 1 transparent node per level, we need to make sure to compare any further transparent
//        // nodes with the one we found here
//        baseNode = *nodeIt;
//      }
//    } // end: for
//
//    // move elements from childNodes to unprocessedNodes: childNodes is empty afterwards
//    unprocessedNodes = std::move(childNodes);
//
//    // reset #children counter back to default value
//    numChildrenPerNode = -1;
//  }

  // if we processed all nodes and did not abort in between due to failed batching compatibility, the node rooted
  // at subtreeRoot is considered as batchable
  //bool isBatchable = qReading.empty() && qWriting.empty();


}
void SpecialExpressionBatcher::visit(AbstractStatement &) {
    throw std::runtime_error("Cannot use the Expression Batcher on statements.");
}