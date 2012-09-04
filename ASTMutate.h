#include "clang/Basic/LLVM.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"

namespace clang {
  enum ACTION { NUMBER, DELETE, INSERT, SWAP };
  
  ASTConsumer *CreateASTDeleter(int Stmt);
}
