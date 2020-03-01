#ifndef AST_OPTIMIZER_INCLUDE_AST_BLOCK_H_
#define AST_OPTIMIZER_INCLUDE_AST_BLOCK_H_

#include <vector>
#include "AbstractStatement.h"
#include <string>

class Block : public AbstractStatement {
 public:
  Block() = default;

  explicit Block(std::vector<AbstractStatement *> statements);

  ~Block() override = default;

  Block *clone(bool keepOriginalUniqueNodeId) override;

  AbstractNode *cloneFlat() override;

  explicit Block(AbstractStatement *stat);

  [[nodiscard]] json toJson() const override;

  void accept(Visitor &v) override;

  [[nodiscard]] std::string getNodeType() const override;

  [[nodiscard]] std::vector<AbstractStatement *> getStatements() const;

  int getMaxNumberChildren() override;

  bool supportsCircuitMode() override;

  [[nodiscard]] std::string toString(bool printChildren) const override;
};

#endif //AST_OPTIMIZER_INCLUDE_AST_BLOCK_H_
