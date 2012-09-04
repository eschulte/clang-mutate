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
