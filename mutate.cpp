/* mutate.cpp --- Mutate statements in a source file       -*- c++ -*-
 * 
 * Adapted from the very good CIrewriter.cpp tutorial in
 * http://github.com/loarabia/Clang-tutorial
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Rewriters.h"
#include "clang/Rewrite/Rewriter.h"

using namespace clang;

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

class MyASTConsumer : public ASTConsumer
{
 public:
    MyASTConsumer(const char *f);
    virtual bool HandleTopLevelDecl(DeclGroupRef d);

    MyRecursiveASTVisitor rv;
};

MyASTConsumer::MyASTConsumer(const char *f)
{
  rv.ci = new CompilerInstance();
  rv.ci->createDiagnostics(0,NULL);

  TargetOptions to;
  to.Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *pti = TargetInfo::CreateTargetInfo(rv.ci->getDiagnostics(), to);
  rv.ci->setTarget(pti);

  rv.ci->createFileManager();
  rv.ci->createSourceManager(rv.ci->getFileManager());
  rv.ci->createPreprocessor();
  rv.ci->getPreprocessorOpts().UsePredefines = false;

  rv.ci->setASTConsumer(this);

  rv.ci->createASTContext();

  // Initialize rewriter
  rv.Rewrite.setSourceMgr(rv.ci->getSourceManager(), rv.ci->getLangOpts());
  
  const FileEntry *pFile = rv.ci->getFileManager().getFile(f);
  rv.ci->getSourceManager().createMainFileID(pFile);
  rv.ci->getDiagnosticClient().BeginSourceFile(rv.ci->getLangOpts(),
                                           &rv.ci->getPreprocessor());

  // Convert <file>.c to <file_action>.c
  std::string outName (f);
  size_t ext = outName.rfind(".");
  if (ext == std::string::npos)
     ext = outName.length();
  switch(action){
  case NUMBER: outName.insert(ext, "_num"); break;
  case DELETE: outName.insert(ext, "_del"); break;
  case INSERT: outName.insert(ext, "_ins"); break;
  case SWAP:   outName.insert(ext, "_swp"); break;
  }

  llvm::errs() << "output to: " << outName << "\n";
  std::string OutErrorInfo;
  llvm::raw_fd_ostream outFile(outName.c_str(), OutErrorInfo, 0);

  if (OutErrorInfo.empty())
  {
    // Parse the AST
    ParseAST(rv.ci->getPreprocessor(), this, rv.ci->getASTContext());
    rv.ci->getDiagnosticClient().EndSourceFile();

    // Handle Insertion and Swapping
    std::string rep1, rep2;
    switch(action){
    case INSERT:
      rep1 = rv.Rewrite.getRewrittenText(stmt1->getSourceRange());
      rv.Rewrite.InsertText(stmt1->getLocStart(), rep2, true);
      break;
    case SWAP:
      rep1 = rv.Rewrite.getRewrittenText(stmt2->getSourceRange());
      rep2 = rv.Rewrite.getRewrittenText(stmt2->getSourceRange());
      rv.Rewrite.ReplaceText(stmt1->getSourceRange(), rep2);
      rv.Rewrite.ReplaceText(stmt2->getSourceRange(), rep1);
      break;
    }

    // Output file prefix
    outFile << "/* ";
    switch(action){
    case NUMBER: outFile << "numbered"; break;
    case DELETE: outFile << "deleted "  << stmt_id_1; break;
    case INSERT: outFile << "copying "  << stmt_id_1 << " "
                         << "to "       << stmt_id_2; break;
    case SWAP:   outFile << "swapping " << stmt_id_1 << " "
                         << "with "     << stmt_id_2; break;
    }
    outFile << " using `mutate.cpp' */\n\n";

    // Now output rewritten source code
    const RewriteBuffer *RewriteBuf =
      rv.Rewrite.getRewriteBufferFor(rv.ci->getSourceManager().getMainFileID());
    outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
  }
  else
  {
    llvm::errs() << "Cannot open " << outName << " for writing\n";
  }

  outFile.close();
  delete rv.ci;
}

bool MyASTConsumer::HandleTopLevelDecl(DeclGroupRef d)
{
  typedef DeclGroupRef::iterator iter;

  for (iter b = d.begin(), e = d.end(); b != e; ++b)
  { rv.TraverseDecl(*b); }

  return true;
}

int parse_int_from(char *str, int *offset){
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

// TODO: figure out how to include search paths for libraries
int main(int argc, char *argv[])
{
  struct stat sb;
  int offset;

  if (argc != 3) {
    llvm::errs() << "Usage: mutate <action> <filename>\n";
    return 1;
  }

  // check the action
  switch(argv[1][0]){
  case 'n': action=NUMBER; break;
  case 'd': action=DELETE; break;
  case 'i': action=INSERT; break;
  case 's': action=SWAP;   break;
  default:
    llvm::errs() <<
      "Usage: mutate <action> <filename>\n"
      "\n"
      "Invalid action specified, use one of the following.\n"
      "  n ------------ number all statements\n"
      "  d:<n1> ------- delete the statement numbered n1\n"
      "  i:<n1>:<n2> -- insert statement n1 before statement numbered n2\n"
      "  s:<n1>:<n2> -- swap statements n1 and n2\n";
    return 1;
  }

  if(action != NUMBER) {
    offset=2;
    stmt_id_1 = parse_int_from(argv[1], &offset);
    if(action != DELETE) {
      stmt_id_2 = parse_int_from(argv[1], &offset);
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

  // check the file
  if (stat(argv[2], &sb) == -1)
  {
    perror(argv[2]);
    exit(EXIT_FAILURE);
  }

  MyASTConsumer *astConsumer = new MyASTConsumer(argv[2]);

  return 0;
}
