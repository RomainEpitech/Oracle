#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "history_manager.h"

void create_history_directory() {
    struct stat st = {0};
    
    if (stat("history", &st) == -1) {
        mkdir("history", 0700);
    }
}

void start_new_conversation(const char *conversation_id) {
    create_history_directory();

    char filename[256];
    snprintf(filename, sizeof(filename), "history/conversation_%s.json", conversation_id);
    FILE *file = fopen(filename, "w");
    if (file != NULL) {
        json_object *root_obj = json_object_new_object();
        json_object *messages_array = json_object_new_array();

        json_object_object_add(root_obj, "conversation_id", json_object_new_string(conversation_id));
        json_object_object_add(root_obj, "messages", messages_array);

        fprintf(file, "%s", json_object_to_json_string(root_obj));
        json_object_put(root_obj);
        fclose(file);
    } else {
        perror("Failed to create conversation file");
    }
}

void save_message(const char *role, const char *message, const char *conversation_id) {
    char filename[256];
    snprintf(filename, sizeof(filename), "history/conversation_%s.json", conversation_id);
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open conversation file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    json_object *root_obj = json_tokener_parse(data);
    free(data);

    if (root_obj == NULL) {
        perror("Failed to parse JSON");
        return;
    }

    json_object *messages_array;
    if (!json_object_object_get_ex(root_obj, "messages", &messages_array)) {
        perror("Failed to get messages array from JSON");
        json_object_put(root_obj);
        return;
    }

    json_object *new_message = json_object_new_object();
    json_object_object_add(new_message, "role", json_object_new_string(role));
    json_object_object_add(new_message, "message", json_object_new_string(message));
    json_object_array_add(messages_array, new_message);

    file = fopen(filename, "w");
    if (file != NULL) {
        fprintf(file, "%s", json_object_to_json_string(root_obj));
        fclose(file);
    } else {
        perror("Failed to open conversation file for writing");
    }

    json_object_put(root_obj);
}
