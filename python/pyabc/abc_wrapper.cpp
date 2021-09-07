#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <ast_opt/parser/Parser.h>
#include <ast_opt/parser/Errors.h>
#include <ast_opt/visitor/ProgramPrintVisitor.h>
#include "ast_opt/compiler/Compiler.h"
#include "ast_opt/runtime/Cleartext.h"

namespace py = pybind11;

/// Class storing a program and intermediate state. E.g., it stores a pre-compiled program AST to avoid
/// having to pass a unique pointer to Python.
template<class T>
class Program {
 private:
  std::unique_ptr<AbstractNode> programAst;

 public:
  /// Create a program and pre-process the given JSON to a proper ABC AST.
  /// \param program JSON version of ABC AST
  explicit Program(const std::string program) {
    programAst = Parser::parseJson(program);
  }

  /// Execute the compiled program on the given inputs and outputs
  // XXX: This assumes the result is in clear text.
  std::vector<T> execute(std::string inputs, std::vector<std::string> outputIdentifiers) {
    // Cloning the programAst is necessary to be able to execute the same program multiple times (which uses the same
    // program AST)
    auto result = Compiler::compileJson(programAst->clone(), inputs, outputIdentifiers);

    std::vector<T> result_vec;

    for (const auto &[identifier, cipherClearText] : result) {

      if (auto cleartextInt = dynamic_cast<Cleartext<T> *>(cipherClearText.get())) {   // result is a cleartext
        auto cleartextData = cleartextInt->getData();
        result_vec.push_back(cleartextData[0]);
      }
      else {
        stork::runtime_error("Currently, only the dummy ciphertext factory and cleartext results are implemented!");
      }
    }
    return result_vec;
  }

  /// Convert the ABC AST to CPP pseudo-code
  std::string to_cpp_string() {
    std::stringstream ss;
    ProgramPrintVisitor v(ss);
    programAst->accept(v);
    return ss.str();
  }
};

PYBIND11_MODULE(_abc_wrapper, m) {
  m.doc() = "Wrapper to export ABC's functionality to read and execute an AST from a JSON file.";

  py::class_<Program<int>>(m, "IntProgram")
      .def(py::init<const std::string>(), "Create a program and pre-process the given JSON to a proper ABC AST.")
      .def("execute", &Program<int>::execute, "Execute the compiled program on the given inputs and outputs")
      .def("to_cpp_string", &Program<int>::to_cpp_string, "Convert the ABC AST to CPP pseudo-code");
}
