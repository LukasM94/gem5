#include <stdio.h>

#define PAGE_SIZE (1<<12)
#define SIZE 0x100
#define LARGE SIZE * PAGE_SIZE

size_t array[LARGE];

int main()
{
  printf("main: &array[LARGE - 1] <%p>\n", &array[LARGE - 1]);
  printf("main: start filling array\n");
  for (int i = 0; i < LARGE; i+= PAGE_SIZE)
  {
    array[i] = i / PAGE_SIZE;
  }
  printf("main: start reading array\n");
  for (int i = 0; i < LARGE; i+= PAGE_SIZE)
  {
    printf("%d\n", array[i]);
  }
}
