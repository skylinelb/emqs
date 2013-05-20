#include <stdio.h>
#include <stdlib.h>

struct people{
   int age;
   char name[10];
};
typedef struct people People;

int main(int argc, char **argv)
{
   People *p;
   int i;

   p = malloc(sizeof(People) * 5);
   for(i = 0; i <5; i++) {
      p[i].age = i;
   }

   for(i = 0; i <5; i++) {
       printf("%d\n", p[i].age);
   }
   free(p);
}
