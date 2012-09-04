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

cl::opt<std::string> MutOpt(
  cl::Positional,
  cl::desc("<mut-op>"));

namespace {
class ActionFactory {
public:
  clang::ASTConsumer *newASTConsumer() {
  errs() << "MutOpt: " << MutOpt << "\n";
  return clang::CreateASTMutator(MutOpt);
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

//===----------------------------------------------------------------------===//
//
//  The following implements basic mutation operations over clang ASTs.
//
//===----------------------------------------------------------------------===//

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
