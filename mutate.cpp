/* mutate.cpp --- Mutate statements in a source file       -*- c++ -*-
 * 
 * Adapted from the very good CIrewriter.cpp tutorial in
 * http://github.com/loarabia/Clang-tutorial
 */
#include "ASTMutator.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

using namespace clang;

class MyASTConsumer : public ASTConsumer
{
 public:
    MyASTConsumer(ACTION action, const char *f, int stmt_id_1, int stmt_id_2);
    virtual bool HandleTopLevelDecl(DeclGroupRef d);

    MyRecursiveASTVisitor rv;
};

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
  ACTION action;
  unsigned int stmt_id_1, stmt_id_2;
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

  MyASTConsumer *astConsumer =
    new MyASTConsumer(action, argv[2], stmt_id_1, stmt_id_2);

  return 0;
}
