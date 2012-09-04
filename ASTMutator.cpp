#include "ASTMutator.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
using namespace clang;

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


// ASTConsumer *clang::CreateMyRecursiveASTVisitor(StringRef MutOp,
//                                                 std::list<int> IDs) {
//   MyRecursiveASTVisitor Visitor = new MyRecursiveASTVisitor(Mutop);
  
//   // Parse Operation
//   switch(MutOp[0]){
//   case 'n': Visitor::action=NUMBER; break;
//   case 'd': Visitor::action=DELETE; break;
//   case 'i': Visitor::action=INSERT; break;
//   case 's': Visitor::action=SWAP;   break;
//   default: llvm::errs() << "bad action: " << MutOp << "\n";
//   }

//   // Parse Statement IDs
//   if(Visitor::action != NUMBER) {
//     Visitor::stmt_id_1 = IDs[0];
//     if(Visitor::action != DELETE) {
//       Visitor::stmt_id_2 = IDs[1];
//     }
//   }

//   return Visitor;
// }

MyASTConsumer::MyASTConsumer(ACTION action, const char *f,
                             int stmt_id_1, int stmt_id_2)
{
  rv.action = action;
  rv.stmt_id_1 = stmt_id_1;
  rv.stmt_id_2 = stmt_id_2;

  rv.ci = new CompilerInstance();
  rv.ci->createDiagnostics(0,NULL);

  TargetOptions to;
  to.Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *pti = TargetInfo::CreateTargetInfo(rv.ci->getDiagnostics(), to);
  rv.ci->setTarget(pti);

  rv.ci->createFileManager();
  rv.ci->createSourceManager(rv.ci->getFileManager());
  rv.ci->createPreprocessor();
  rv.ci->getPreprocessorOpts().UsePredefines = true;
  rv.ci->setASTConsumer(this);
  rv.ci->createASTContext();

  HeaderSearch headerSearch(rv.ci->getFileManager(),
                            rv.ci->getDiagnostics(),
                            rv.ci->getLangOpts(),
                            pti);

  HeaderSearchOptions headerSearchOptions;
  
  headerSearchOptions.AddPath(
    "/usr/local/src/llvm/tools/clang/lib/Headers",
    clang::frontend::Angled, 
    false, 
    false, 
    false);

  headerSearchOptions.AddPath(
    "/usr/local/src/llvm/projects/compiler-rt/SDKs/linux/usr/include",
    clang::frontend::Angled, 
    false, 
    false, 
    false);
  
  clang::InitializePreprocessor(rv.ci->getPreprocessor(), 
                                rv.ci->getPreprocessorOpts(),
                                headerSearchOptions,
                                rv.ci->getFrontendOpts());

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
      rep2 = rv.Rewrite.getRewrittenText(rv.stmt2->getSourceRange());
      rv.Rewrite.InsertText(rv.stmt1->getLocStart(), rep2, true);
      break;
    case SWAP:
      rep1 = rv.Rewrite.getRewrittenText(rv.stmt1->getSourceRange());
      rep2 = rv.Rewrite.getRewrittenText(rv.stmt2->getSourceRange());
      rv.Rewrite.ReplaceText(rv.stmt1->getSourceRange(), rep2);
      rv.Rewrite.ReplaceText(rv.stmt2->getSourceRange(), rep1);
      break;
    default: break;
    }

    // Output file prefix
    outFile << "/* ";
    switch(action){
    case NUMBER: outFile << "numbered"; break;
    case DELETE: outFile << "deleted "  << rv.stmt_id_1; break;
    case INSERT: outFile << "copying "  << rv.stmt_id_1 << " "
                         << "to "       << rv.stmt_id_2; break;
    case SWAP:   outFile << "swapping " << rv.stmt_id_1 << " "
                         << "with "     << rv.stmt_id_2; break;
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
