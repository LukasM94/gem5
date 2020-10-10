#include <stdio.h>

#define PAGE_SIZE (1<<12)
#define SIZE 0x1000
#define LARGE SIZE * PAGE_SIZE

size_t array[LARGE];

int main()
{
  printf("main: start filling array\n");
  size_t* ptr = (size_t*)((size_t)array / PAGE_SIZE * PAGE_SIZE + PAGE_SIZE);
  printf("main: ptr <%p>\n", ptr);
  printf("main: &array[LARGE - 1] <%p>\n", &array[LARGE - 1]);
  while (ptr < &array[LARGE - 1])
  {
    *ptr = 0;
    ptr += PAGE_SIZE;
  }
}
