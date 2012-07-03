#include <stdio.h> // <- just delete this line for things to work

int do_math(int input)
{
  int a,b;
  // and do some math
  a = input;
  b = 1;
  return a + b;  
}

int main(int argc, char *argv[])
{
  
  // This is an IF statement w/o braces
  if(argc == 1) puts("hello world.");
  else          puts("be quiet!");
  // Also, print out some other stuff
  puts("other stuff");
  return do_math(-1);
}
