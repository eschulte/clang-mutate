//                                                         -*- C++ -*-
#ifndef DRIVER_ASTMUTATORS_H
#define DRIVER_ASTMUTATORS_H

#include "clang/Basic/LLVM.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"

namespace clang {  

enum ACTION { NUMBER, DELETE, INSERT, SWAP };

// Deletes the specified statement from an AST
ASTConsumer *CreateASTDeleter(int Stmt);

// Number all statements in an AST
ASTConsumer *CreateASTNumberer();

}

#endif
