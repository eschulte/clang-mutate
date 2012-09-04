// #include <stdio.h> removed so we can test w/o having to move the
//                    executable to the clang directory

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
