#include "ASTMutate.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Rewriters.h"
#include "clang/Rewrite/Rewriter.h"
#include "llvm/Module.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Timer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
using namespace clang;

namespace {
  class ASTMutator : public ASTConsumer,
                     public RecursiveASTVisitor<ASTMutator> {
    typedef RecursiveASTVisitor<ASTMutator> base;

  public:
    ASTMutator(raw_ostream *Out = NULL, bool Dump = false,
               ACTION Action = NUMBER,
               int Stmt1 = -1, int Stmt2 = -1)
      : Out(Out ? *Out : llvm::outs()), Dump(Dump), Action(Action),
        Stmt1(Stmt1), Stmt2(Stmt2) {}

    virtual void HandleTranslationUnit(ASTContext &Context) {
      TranslationUnitDecl *D = Context.getTranslationUnitDecl();
      TraverseDecl(D);
    };
    
    Rewriter Rewrite;

    bool SelectStmt(Stmt *s)
    { return ! isa<DefaultStmt>(s); }

    void NumberStmt(Stmt *s)
    {
      char label[24];
      unsigned EndOff;
      SourceLocation END = s->getLocEnd();

      sprintf(label, "/* %d[ */", counter);
      Rewrite.InsertText(s->getLocStart(), label, false);
      sprintf(label, "/* ]%d */", counter);
  
      // Adjust the end offset to the end of the last token, instead
      // of being the start of the last token.
      EndOff = Lexer::MeasureTokenLength(END,
                                         Rewrite.getSourceMgr(),
                                         Rewrite.getLangOpts());

      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
    }

    void DeleteStmt(Stmt *s)
    {
      char label[24];
      if(counter == Stmt1) {
        sprintf(label, "/* deleted:%d */", counter);
        Rewrite.ReplaceText(s->getSourceRange(), label);
      }
    }

    void SaveStmt(Stmt *s)
    {
      if (counter == Stmt1) {
        stmt_set_1 = true;
        stmt1 = s;
      }
      if (counter == Stmt2) {
        stmt_set_2 = true;
        stmt2 = s;
      }
    }

    bool VisitStmt(Stmt *s) {
      if (SelectStmt(s)) {
        switch(Action) {
        case NUMBER: NumberStmt(s); break;
        case DELETE: DeleteStmt(s); break;
        case INSERT:
        case SWAP:     SaveStmt(s); break;
        }
      }
      counter++;
      return true;
    }
    
  private:
    raw_ostream &Out;
    bool Dump;
    ACTION Action;
    int Stmt1, Stmt2;
    unsigned int counter;
    Stmt *stmt1, *stmt2;
    bool stmt_set_1, stmt_set_2;
  };
}

ASTConsumer *CreateASTDeleter(int Stmt){
  return new ASTMutator(0, /*Dump=*/ true, DELETE, Stmt, -1);
}
