#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Rewriters.h"
#include "clang/Rewrite/Rewriter.h"

using namespace clang;
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

enum ACTION { NUMBER, DELETE, INSERT, SWAP };

Stmt *stmt1, *stmt2;
bool stmt_set_1, stmt_set_2 = false;
unsigned int action, stmt_id_1, stmt_id_2;
unsigned int counter = 0;

class MyRecursiveASTVisitor
    : public RecursiveASTVisitor<MyRecursiveASTVisitor>
{
 public:
  bool SelectStmt(Stmt *s);
  void NumberStmt(Stmt *s);
  void DeleteStmt(Stmt *s);
  void   SaveStmt(Stmt *s);
  bool  VisitStmt(Stmt *s);

  Rewriter Rewrite;
  CompilerInstance *ci;
};

bool MyRecursiveASTVisitor::SelectStmt(Stmt *s)
{ return ! isa<DefaultStmt>(s); }

void MyRecursiveASTVisitor::NumberStmt(Stmt *s)
{
  char label[24];
  unsigned EndOff;
  SourceLocation END = s->getLocEnd();

  sprintf(label, "/* %d[ */", counter);
  Rewrite.InsertText(s->getLocStart(), label, false);
  sprintf(label, "/* ]%d */", counter);
  
  // Adjust the end offset to the end of the last token, instead of being the
  // start of the last token.
  EndOff = Lexer::MeasureTokenLength(END,
                                     Rewrite.getSourceMgr(),
                                     Rewrite.getLangOpts());

  Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
}

void MyRecursiveASTVisitor::DeleteStmt(Stmt *s)
{
  char label[24];
  if(counter == stmt_id_1) {
    sprintf(label, "/* deleted:%d */", counter);
    Rewrite.ReplaceText(s->getSourceRange(), label);
  }
}

void MyRecursiveASTVisitor::SaveStmt(Stmt *s)
{
  if (counter == stmt_id_1) {
    stmt_set_1 = true;
    stmt1 = s;
  }
  if (counter == stmt_id_2) {
    stmt_set_2 = true;
    stmt2 = s;
  }
}

bool MyRecursiveASTVisitor::VisitStmt(Stmt *s) {
  if (SelectStmt(s)) {
    switch(action) {
    case NUMBER: NumberStmt(s); break;
    case DELETE: DeleteStmt(s); break;
    case INSERT:
    case SWAP:     SaveStmt(s); break;
    }
  }
  counter++;
  return true;
}
