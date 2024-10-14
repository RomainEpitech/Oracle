#include "../includes/oracle.h"
#include <stdlib.h>
#include <string.h>

static const char* api_key = NULL;
#define API_URL "https://api.openai.com/v1/chat/completions"

void load_env_variables(void) {
    if (load_env(".env") != 0) {
        fprintf(stderr, "Failed to load .env file\n");
        exit(1);
    }
    api_key = get_env("API_KEY");
    if (!api_key) {
        fprintf(stderr, "API_KEY not found in .env file\n");
        exit(1);
    }
}

const char* get_api_key(void) {
    if (!api_key) {
        load_env_variables();
    }
    // printf("API Key: %s\n", api_key);
    return api_key;
}

char* oracle_process(const char* input) {
    return chat_with_gpt(input);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char* chat_with_gpt(const char* input) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", get_api_key());
        headers = curl_slist_append(headers, auth_header);

        char request_body[1024];
        snprintf(request_body, sizeof(request_body),
                    "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]}",
                    input);

        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return strdup("Une erreur s'est produite lors de la communication avec l'API.");
        }

        struct json_object *parsed_json;
        struct json_object *choices;
        struct json_object *first_choice;
        struct json_object *message;
        struct json_object *content;

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return strdup("Une erreur s'est produite lors de la communication avec l'API.");
        }
        parsed_json = json_tokener_parse(chunk.memory);

        if (json_object_object_get_ex(parsed_json, "choices", &choices) &&
            json_object_get_type(choices) == json_type_array &&
            json_object_array_length(choices) > 0) {
            
            first_choice = json_object_array_get_idx(choices, 0);
            
            if (json_object_object_get_ex(first_choice, "message", &message) &&
                json_object_object_get_ex(message, "content", &content)) {
                
                const char *content_str = json_object_get_string(content);
                char *response = strdup(content_str);
                
                json_object_put(parsed_json);
                free(chunk.memory);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                
                return response;
            }
        }

        json_object_put(parsed_json);
        free(chunk.memory);
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        return strdup("Erreur lors du parsing de la réponse de l'API.");
    }

    curl_global_cleanup();
    return strdup("Erreur lors de l'initialisation de CURL.");
}