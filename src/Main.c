#include "../includes/Oracle.h"
#include "../includes/history_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>

#define ORACLE_MAX_INPUT 1000

char* generate_conversation_id() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    static char id_str[20];
    strftime(id_str, sizeof(id_str), "%Y%m%d%H%M%S", t);
    return id_str;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    load_env_variables();
    
    char input[ORACLE_MAX_INPUT];
    char* conversation_id = generate_conversation_id();
    
    start_new_conversation(conversation_id);

    printf("Historique de la conversation %s:\n", conversation_id);
    load_conversation_history(conversation_id);
    
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

        save_message("user", input, conversation_id);
        char* response = oracle_process(input);
        save_message("oracle", response, conversation_id);
        
        printf("%s\n", response);
        free(response);
    }
    
    printf("Merci d'avoir utilisé Oracle. Au revoir !\n");
    
    curl_global_cleanup();
    return 0;
}
