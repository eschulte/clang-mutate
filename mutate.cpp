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

  MutatorASTConsumer *astConsumer =
    new MutatorASTConsumer(action, argv[2], stmt_id_1, stmt_id_2);

  return 0;
}
