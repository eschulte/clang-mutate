#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

using namespace clang::tooling;
using namespace llvm;

cl::opt<std::string> BuildPath(
  cl::Positional,
  cl::desc("<build-path>"));

cl::opt<std::string> MutOpt(
  cl::Positional,
  cl::desc("<mut-op>"));

cl::list<std::string> SourcePaths(
  cl::Positional,
  cl::desc("<source0> [... <sourceN>]"),
  cl::OneOrMore);

static cl::extrahelp MoreHelp(
  "   mut-op  - should be one of the following;\n"
  "             n       - number all statements\n"
  "             d:#1    - delete the statement numbered #1\n"
  "             i:#1:#2 - insert statement #1 before statement numbered #2\n"
  "             s:#1:#2 - swap statements #1 and #2\n");

int main(int argc, const char **argv) {
  llvm::OwningPtr<CompilationDatabase> Compilations(
    FixedCompilationDatabase::loadFromCommandLine(argc, argv));
  cl::ParseCommandLineOptions(argc, argv);
  if (!Compilations) {
    std::string ErrorMessage;
    Compilations.reset(CompilationDatabase::loadFromDirectory(BuildPath,
                                                              ErrorMessage));
    if (!Compilations)
      llvm::report_fatal_error(ErrorMessage);
  }
  ClangTool Tool(*Compilations, SourcePaths);
  return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());
}
