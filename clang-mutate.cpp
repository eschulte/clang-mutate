//===--------------- clang-mutate.cpp - Clang mutation tool ---------------===//
//
// Adopted from ClangCheck.cpp by Eric Schulte
//
//===----------------------------------------------------------------------===//
//
//  This file implements a mutation tool that runs the
//  clang::MutateAction over a number of translation units.
//
//  This tool uses the Clang Tooling infrastructure, see
//    http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html
//  for details on setting it up with LLVM source tree.
//
//===----------------------------------------------------------------------===//
#include "ASTMutate.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
    "\tFor example, to number all statements in a file, use:\n"
    "\n"
    "\t  ./mutate2 n file.c\n"
    "\n"
);

cl::opt<std::string> MutOp(
  cl::Positional,
  cl::desc("<mut-op>"));

static cl::opt<bool> Number("number", "number all statements");
static cl::opt<bool> Delete("delete", "delete stmt-1");
static cl::opt<bool> Insert("insert", "insert a copy of stmt-1 after stmt-2");
static cl::opt<bool>   Swap("swap",   "Swap stmt-1 and stmt-2");
static cl::opt<int>   Stmt1("stmt-1", "first statement argument to mut-opt");
static cl::opt<int>   Stmt2("stmt-2", "second statement argument to mut-opt");

namespace {
class ActionFactory {
public:
  clang::ASTConsumer *newASTConsumer() {
    // if (Number)
    //   return clang::CreateASTNumberer();
    if (Delete)
      return clang::CreateASTDeleter(Stmt1);
    // if (Insert)
    //   return clang::CreateASTInserter(Stmt1, Stmt2);
    // if (Swap)
    //   return clang::CreateASTSwapper(Stmt1, Stmt2);
    errs() << "must supply one of the [number,delete,insert,swap] options\n";
    exit(EXIT_FAILURE);
  }
};
}

int main(int argc, const char **argv) {
  ActionFactory Factory;
  CommonOptionsParser OptionsParser(argc, argv);
  ClangTool Tool(OptionsParser.GetCompilations(),
                 OptionsParser.GetSourcePathList());
  return Tool.run(newFrontendActionFactory(&Factory));
}
