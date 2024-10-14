#include "../includes/Oracle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define ORACLE_MAX_INPUT 1000

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    load_env_variables();
    
    char input[ORACLE_MAX_INPUT];
    
    printf("Bienvenue dans Oracle. Que puis-je faire pour vous ?\n");
    
    while (1) {
        printf("> ");
        if (fgets(input, ORACLE_MAX_INPUT, stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "exit") == 0) {
            break;
        }
        
        char* response = oracle_process(input);
        printf("%s\n", response);
        free(response);
    }
    
    printf("Merci d'avoir utilisé Oracle. Au revoir !\n");
    
    curl_global_cleanup();
    return 0;
}