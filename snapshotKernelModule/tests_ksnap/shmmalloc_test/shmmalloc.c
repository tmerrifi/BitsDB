
#include <mm.h>

int main(){

  MM_create(1000, "simple_test");
  int * my_int = MM_malloc(sizeof(int));
  *my_int = 10;
}
