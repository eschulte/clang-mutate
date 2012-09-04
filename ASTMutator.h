#include "clang/Basic/LLVM.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/FileSystemOptions.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Frontend/HeaderSearchOptions.h"
#include "clang/Frontend/PreprocessorOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/Parser.h"
#include "clang/Rewrite/Rewriter.h"
#include "clang/Rewrite/Rewriters.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/Sema.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include <list>

namespace clang {
  enum ACTION { NUMBER, DELETE, INSERT, SWAP };
  
  class MutatorASTVisitor : public ASTConsumer,
                            public RecursiveASTVisitor<MutatorASTVisitor>
  {
  public:
    bool SelectStmt(Stmt *s);
    void NumberStmt(Stmt *s);
    void DeleteStmt(Stmt *s);
    void   SaveStmt(Stmt *s);
    bool  VisitStmt(Stmt *s);

    Rewriter Rewrite;
    CompilerInstance *ci;

    unsigned int action, stmt_id_1, stmt_id_2;
    Stmt *stmt1, *stmt2;

    private:
      bool stmt_set_1, stmt_set_2;
      unsigned int counter;
  };

  class MutatorASTConsumer : public ASTConsumer
  {
  public:
    MutatorASTConsumer(ACTION action, const char *f, int stmt_id_1, int stmt_id_2);
    virtual bool HandleTopLevelDecl(DeclGroupRef d);

    MutatorASTVisitor rv;
  };
}
