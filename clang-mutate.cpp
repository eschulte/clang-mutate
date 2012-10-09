//===--------------- clang-mutate.cpp - Clang mutation tool ---------------===//
//
// Copyright (C) 2012 Eric Schulte
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
    "Example Usage:\n"
    "\n"
    "\tto number all statements in a file, use:\n"
    "\n"
    "\t  ./clang-mutate -number file.c\n"
    "\n"
    "\tor to delete the 12th statement, use:\n"
    "\n"
    "\t  ./clang-mutate -delete -stmt1=12 file.c\n"
    "\n"
);

static cl::opt<bool> Number("number", cl::desc("number all statements"));
static cl::opt<bool> Delete("delete", cl::desc("delete stmt1"));
static cl::opt<bool> Insert("insert", cl::desc("copy stmt1 to after stmt2"));
static cl::opt<bool>   Swap("swap",   cl::desc("Swap stmt1 and stmt2"));
static cl::opt<int>   Stmt1("stmt1",  cl::desc("statement 1 for mutation ops"));
static cl::opt<int>   Stmt2("stmt2",  cl::desc("statement 2 for mutation ops"));

namespace {
class ActionFactory {
public:
  clang::ASTConsumer *newASTConsumer() {
    if (Number)
      return clang::CreateASTNumberer();
    if (Delete)
      return clang::CreateASTDeleter(Stmt1);
    if (Insert)
      return clang::CreateASTInserter(Stmt1, Stmt2);
    if (Swap)
      return clang::CreateASTSwapper(Stmt1, Stmt2);
    errs() << "Must supply one of the [number,delete,insert,swap] options.\n";
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
