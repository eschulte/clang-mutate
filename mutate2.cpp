#include "clang/AST/ASTConsumer.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "ASTMutator.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;

ACTION parse_mut_op(StringRef MutOp);
int parse_int_from(StringRef str, int *offset);

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
    "\tFor example, to number all statements in a file, use:\n"
    "\n"
    "\t  ./mutate2 n file.c\n"
    "\n"
);

cl::opt<std::string> MutOp(
  cl::Positional,
  cl::desc("<mut-op>"));

namespace {
class ActionFactory {
public:
  clang::ASTConsumer *newASTConsumer() {
  ACTION action = parse_mut_op(MutOp);
  int offset, stmt_id_1, stmt_id_2;
  errs() << "MutOp: " << MutOp << "\n";

  // Record the statement IDs
  offset=2;
  switch(action){
  case DELETE:
    stmt_id_1 = parse_int_from(MutOp, &offset); break;
  case INSERT:
  case SWAP:
    stmt_id_1 = parse_int_from(MutOp, &offset);
    stmt_id_2 = parse_int_from(MutOp, &offset);
  default: break;
  }
  
  switch(action){
  case NUMBER: llvm::errs() << "numbering\n"; break;
  case DELETE: llvm::errs() << "deleting " << stmt_id_1 << "\n"; break;
  case INSERT: llvm::errs() << "copying "  << stmt_id_1 << " "
                            << "to "       << stmt_id_2 << "\n"; break;
  case SWAP:   llvm::errs() << "swapping " << stmt_id_1 << " "
                            << "with "     << stmt_id_2 << "\n"; break;
  }

  return new clang::MutatorASTConsumer(action, "hello.c", stmt_id_1, stmt_id_2);
  }
};
}

int main(int argc, const char **argv) {
  ActionFactory Factory;
  CommonOptionsParser OptionsParser(argc, argv);
  ClangTool Tool(OptionsParser.GetCompilations(),
                 OptionsParser.GetSourcePathList());
  return Tool.run(newFrontendActionFactory(&Factory));
}

//-------------------------------------------------------------------
// Support Functions

int parse_int_from(StringRef str, int *offset){
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

ACTION parse_mut_op(StringRef MutOp){
  switch(MutOp[0]){
  case 'n': return NUMBER;
  case 'd': return DELETE;
  case 'i': return INSERT;
  case 's': return SWAP;
  default:
    llvm::errs() <<
      "Usage: mutate <action> <filename>\n"
      "\n"
      "Invalid action specified, use one of the following.\n"
      "  n ------------ number all statements\n"
      "  d:<n1> ------- delete the statement numbered n1\n"
      "  i:<n1>:<n2> -- insert statement n1 before statement numbered n2\n"
      "  s:<n1>:<n2> -- swap statements n1 and n2\n";
    exit(EXIT_FAILURE);
  }
}
