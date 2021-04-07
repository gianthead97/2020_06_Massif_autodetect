#include <stdlib.h>

void g(void)
{
  malloc(4000);
}

void f(void)
{
  malloc(2000);
  g();
}

void huge(void)
{
  malloc(15000);
}

int main(void)
{
  int i;
  int* a[10];

  for (i = 0; i < 5; i++) {
    a[i] = malloc(1000);
  }

  void *m = malloc(5000);

  for(;i<10;i++){
    a[i] = malloc(1000);
  }

  f();
  g(); 

  huge();

  for (i = 0; i < 10; i++) {
    free(a[i]);
  }

  huge();

  f();
  g(); 

  return 0;
}
