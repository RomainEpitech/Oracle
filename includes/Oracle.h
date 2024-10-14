#ifndef ORACLE_H
#define ORACLE_H

#include <curl/curl.h>
#include <json-c/json.h>
#include "env_loader.h"

// Structure pour stocker la réponse de l'API
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Fonction pour charger les variables d'environnement
void load_env_variables(void);

// Fonction pour obtenir la clé API
const char* get_api_key(void);

// Fonction principale d'Oracle
char* oracle_process(const char* input);

// Fonction pour interagir avec l'API ChatGPT
char* chat_with_gpt(const char* input);

#endif