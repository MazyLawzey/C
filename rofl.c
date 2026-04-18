#include <stdio.h>

int main() {
    int x = 0;
    while (x <= 100){
        x++;
        printf("%3d\n", x);
    }
    if (x == 101){
        printf("x = 101!\n");
    }
}
