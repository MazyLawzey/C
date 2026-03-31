#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char soobchenie[100];
int text();

int main() {
    text();
    printf("%s\n", soobchenie);
    return 0;
}

int text() {
    printf("Введите сообщение: ");
    fgets(soobchenie, sizeof(soobchenie), stdin);
    return 0;
}
