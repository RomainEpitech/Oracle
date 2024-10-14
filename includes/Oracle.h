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

extern MarkdownFile *docs;
extern int num_docs;

void load_markdown_docs();
void load_env_variables(void);
const char* get_api_key(void);
char* oracle_process(const char* input);
char* chat_with_gpt(const char* input);
char* read_markdown_file(const char* filepath);

#endif
