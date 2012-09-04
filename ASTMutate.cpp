// Copyright (C) 2012 Eric Schulte
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
      // Setup
      Counter=0;
      Rewrite.setSourceMgr(Context.getSourceManager(),
                           Context.getLangOpts());
      // Run Recursive AST Visitor
      TraverseDecl(D);
      // Finish Up
      switch(Action){
      case INSERT:
        Rewritten2 = Rewrite.getRewrittenText(Range2);
        Rewrite.InsertText(Range1.getBegin(), (Rewritten2+" "), true);
        break;
      case SWAP:
        Rewritten1 = Rewrite.getRewrittenText(Range1);
        Rewritten2 = Rewrite.getRewrittenText(Range2);
        Rewrite.ReplaceText(Range1, Rewritten2);
        Rewrite.ReplaceText(Range2, Rewritten1);
        break;
      default: break;
      }
      OutputRewritten(Context);
    };
    
    Rewriter Rewrite;

    bool SelectStmt(Stmt *s)
    { return ! isa<DefaultStmt>(s); }

    void NumberStmt(Stmt *s)
    {
      char label[24];
      unsigned EndOff;
      SourceLocation END = s->getLocEnd();

      sprintf(label, "/* %d[ */", Counter);
      Rewrite.InsertText(s->getLocStart(), label, false);
      sprintf(label, "/* ]%d */", Counter);
  
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
      // TODO: Could check if next token is semicolon and remove it
      //       depending on the context.  Alternately maybe clang has
      //       a transform which removed superfluous semicolons.
      //       
      // SourceLocation Beg = s->getLocStart();
      // SourceLocation End = s->getLocEnd();
      if(Counter == Stmt1) {
        sprintf(label, "/* deleted:%d */", Counter);
        // Rewrite.ReplaceText(SourceRange(Beg,End), label);
        Rewrite.ReplaceText(s->getSourceRange(), label);
      }
    }

    void SaveStmt(Stmt *s)
    {
      if (Counter == Stmt1)
        Range1 = s->getSourceRange();
      if (Counter == Stmt2)
        Range2 = s->getSourceRange();
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
      Counter++;
      return true;
    }

    void OutputRewritten(ASTContext &Context) {
      // Output file prefix
      Out << "/* ";
      switch(Action){
      case NUMBER: Out << "numbered"; break;
      case DELETE: Out << "deleted "  << Stmt1; break;
      case INSERT: Out << "copying "  << Stmt1 << " to "   << Stmt2; break;
      case SWAP:   Out << "swapping " << Stmt1 << " with " << Stmt2; break;
      }
      Out << " using clang-mutate */\n";

      // Now output rewritten source code
      const RewriteBuffer *RewriteBuf = 
        Rewrite.getRewriteBufferFor(Context.getSourceManager().getMainFileID());
      Out << std::string(RewriteBuf->begin(), RewriteBuf->end());
    }
    
  private:
    raw_ostream &Out;
    bool Dump;
    ACTION Action;
    int Stmt1, Stmt2;
    unsigned int Counter;
    SourceRange Range1, Range2;
    std::string Rewritten1, Rewritten2;
  };
}

ASTConsumer *clang::CreateASTNumberer(){
  return new ASTMutator(0, /*Dump=*/ true, NUMBER, -1, -1);
}

ASTConsumer *clang::CreateASTDeleter(int Stmt){
  return new ASTMutator(0, /*Dump=*/ true, DELETE, Stmt, -1);
}

ASTConsumer *clang::CreateASTInserter(int Stmt1, int Stmt2){
  return new ASTMutator(0, /*Dump=*/ true, INSERT, Stmt1, Stmt2);
}

ASTConsumer *clang::CreateASTSwapper(int Stmt1, int Stmt2){
  return new ASTMutator(0, /*Dump=*/ true, SWAP, Stmt1, Stmt2);
}
