#ifndef ORACLE_H
#define ORACLE_H

#include <curl/curl.h>
#include <json-c/json.h>
#include "env_loader.h"

struct MemoryStruct {
    char *memory;
    size_t size;
};

typedef struct {
    char *filename;
    char *content;
} MarkdownFile;

typedef struct {
    char* content;
    float relevance;
} RelevantInfo;

typedef struct {
    char* thought;
    RelevantInfo* info;
    int info_count;
} Reflection;

extern MarkdownFile *docs;
extern int num_docs;

void load_markdown_docs();
void load_env_variables(void);
const char* get_api_key(void);
char* oracle_process(const char* input);
char* chat_with_gpt(const char* input);
char* read_markdown_file(const char* filepath);
char* search_in_markdown_with_similarity(const char* content, const char* query);
Reflection* reflect_on_query(const char* query);
char* formulate_response(Reflection* reflection);
void free_reflection(Reflection* reflection);
int levenshtein_distance(const char *s1, const char *s2);
static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp);
char* find_relevant_code(const char* query);

#endif