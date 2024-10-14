#include "../includes/env_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_KEY 128
#define MAX_VALUE 896

static char env_vars[MAX_LINE][2][MAX_VALUE];
static int env_count = 0;

int load_env(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* key = line;
            char* value = eq + 1;
            // Supprimer les espaces en début et fin de chaîne
            while (*key && (*key == ' ' || *key == '\t')) key++;
            char* end = key + strlen(key) - 1;
            while (end > key && (*end == ' ' || *end == '\t' || *end == '\n')) *end-- = '\0';
            
            while (*value && (*value == ' ' || *value == '\t')) value++;
            end = value + strlen(value) - 1;
            while (end > value && (*end == ' ' || *end == '\t' || *end == '\n')) *end-- = '\0';

            if (strlen(key) > 0 && strlen(value) > 0) {
                strncpy(env_vars[env_count][0], key, MAX_KEY);
                strncpy(env_vars[env_count][1], value, MAX_VALUE);
                env_count++;
            }
        }
    }

    fclose(file);
    return 0;
}

const char* get_env(const char* key) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_vars[i][0], key) == 0) {
            return env_vars[i][1];
        }
    }
    return NULL;
}