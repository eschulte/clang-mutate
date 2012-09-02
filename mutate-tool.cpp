#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Rewriter.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <cstdio>

// #include <sys/types.h>
// #include <sys/stat.h>
// #include "clang/Lex/Preprocessor.h"
// #include "clang/Basic/Diagnostic.h"
// #include "clang/Parse/ParseAST.h"
// #include "clang/Rewrite/Rewriters.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

enum ACTION { NUMBER, DELETE, INSERT, SWAP };

Stmt *stmt1, *stmt2;
bool stmt_set_1, stmt_set_2 = false;
unsigned int action, stmt_id_1, stmt_id_2;
unsigned int counter = 0;

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

int parse_int_from(std::string str, int *offset){
  char buffer[12];
  char c;
  int i=-1;
  do {
    c = str[(*offset)];
    ++(*offset);
    ++i;
  } while((c >= '0') && (c <= '9') && (buffer[i] = c));
  buffer[i] = '\0';
  if (i == 0)
  {
    llvm::errs() << "(additional) statement number required\n";
    exit(EXIT_FAILURE);
  }
  return atoi(buffer);
}

void check_mut_opt(std::string str){
  int offset;

  // check the action
  switch(str[0]){
  case 'n': action=NUMBER; break;
  case 'd': action=DELETE; break;
  case 'i': action=INSERT; break;
  case 's': action=SWAP;   break;
  default: llvm::report_fatal_error("try ./mutation-tool -help\n");
  }

  if(action != NUMBER) {
    offset=2;
    stmt_id_1 = parse_int_from(str, &offset);
    if(action != DELETE) {
      stmt_id_2 = parse_int_from(str, &offset);
    }
  }

  switch(action){
  case NUMBER: llvm::errs() << "numbering\n"; break;
  case DELETE: llvm::errs() << "deleting " << stmt_id_1 << "\n"; break;
  case INSERT: llvm::errs() << "copying "  << stmt_id_1 << " "
                            << "to "       << stmt_id_2 << "\n"; break;
  case SWAP:   llvm::errs() << "swapping " << stmt_id_1 << " "
                            << "with "     << stmt_id_2 << "\n"; break;
  }
}

class MutationVisitor
    : public RecursiveASTVisitor<MutationVisitor> {
 public:
  explicit MutationVisitor(ASTContext *Context)
    : Context(Context) {}

  bool SelectStmt(Stmt *s);
  void NumberStmt(Stmt *s);
  void DeleteStmt(Stmt *s);
  void   SaveStmt(Stmt *s);
  bool  VisitStmt(Stmt *s);

  Rewriter Rewrite;
  CompilerInstance *ci;

 private:
  ASTContext *Context;
};

bool MutationVisitor::SelectStmt(Stmt *s)
{ return ! isa<DefaultStmt>(s); }

void MutationVisitor::NumberStmt(Stmt *s)
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

void MutationVisitor::DeleteStmt(Stmt *s)
{
  char label[24];
  if(counter == stmt_id_1) {
    sprintf(label, "/* deleted:%d */", counter);
    Rewrite.ReplaceText(s->getSourceRange(), label);
  }
}

void MutationVisitor::SaveStmt(Stmt *s)
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

bool MutationVisitor::VisitStmt(Stmt *s) {
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

class MyConsumer : public clang::ASTConsumer {
 public:
  explicit MyConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

 private:
  MutationVisitor Visitor;
};

class MutationAction : public clang::ASTFrontendAction {
 public:
  virtual clang::ASTConsumer *CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return new MyConsumer(&Compiler.getASTContext());
  }
};

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv);
  
  // check mutation op
  check_mut_opt(MutOpt);
  
  ClangTool Tool(OptionsParser.GetCompilations(),
                 OptionsParser.GetSourcePathList());
  return Tool.run(newFrontendActionFactory<MutationAction>());
}
