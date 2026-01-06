#include <stdio.h>
int main(){
    char linha [100];
    char palavra_1[100], palavra_2[100];

    while(1){
        fgets(linha, 100, stdin);
        int n_palavras = sscanf(linha, "%s %s", palavra_1, palavra_2);
        if (n_palavras == 2) {
            printf("Primeira palavra: %s\n", palavra_1);
            printf("Segunda palavra: %s\n", palavra_2);
        }
        if (n_palavras == 1) {
            printf("Apenas uma palavra foi lida: %s\n", palavra_1);
        }
        if (n_palavras == 0) {
            printf("Nenhuma palavra foi lida.\n");
        }
    }
}