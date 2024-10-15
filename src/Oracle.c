#include "../includes/Oracle.h"
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static const char* api_key = NULL;
#define API_URL "https://api.openai.com/v1/chat/completions"

MarkdownFile *docs = NULL;
int num_docs = 0;     

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

void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

Reflection* reflect_on_query(const char* query) {
    Reflection* reflection = malloc(sizeof(Reflection));
    reflection->thought = NULL;
    reflection->info = NULL;
    reflection->info_count = 0;

    char analysis_prompt[1024];
    snprintf(analysis_prompt, sizeof(analysis_prompt), "Analyze this query and describe its intent and key concepts: %s", query);
    char* query_analysis = chat_with_gpt(analysis_prompt);

    for (int i = 0; i < num_docs; i++) {
        char* result = search_in_markdown_with_similarity(docs[i].content, query);
        if (result) {
            reflection->info = realloc(reflection->info, (reflection->info_count + 1) * sizeof(RelevantInfo));
            reflection->info[reflection->info_count].content = result;
            reflection->info[reflection->info_count].relevance = 1.0 - ((float)levenshtein_distance(result, query) / (float)strlen(query));
            reflection->info_count++;
        }
    }

    char info_summary[5000] = "";
    for (int i = 0; i < reflection->info_count; i++) {
        strcat(info_summary, reflection->info[i].content);
        strcat(info_summary, "\n");
    }

    char synthesis_prompt[6000];
    snprintf(synthesis_prompt, sizeof(synthesis_prompt),
        "Based on the query: '%s' and the following information:\n%s\n"
        "Synthesize a coherent thought that addresses the query. "
        "Include relevant technical details and consider how to structure a helpful response.",
        query, info_summary);

    reflection->thought = chat_with_gpt(synthesis_prompt);

    free(query_analysis);
    return reflection;
}

char* formulate_response(Reflection* reflection) {
    char prompt[10000];
    snprintf(prompt, sizeof(prompt),
        "Given this thought process: '%s'\n"
        "Formulate a clear, concise, and helpful response to the original query. "
        "Include a brief explanation and, if applicable, a short code example in PHP. "
        "Format the response in a user-friendly manner.",
        reflection->thought);

    return chat_with_gpt(prompt);
}

int contains_word(const char* str, const char* word) {
    char* str_copy = strdup(str);
    char* word_copy = strdup(word);
    to_lowercase(str_copy);
    to_lowercase(word_copy);
    
    int result = strstr(str_copy, word_copy) != NULL;
    
    free(str_copy);
    free(word_copy);
    return result;
}

RelevantContent extract_relevant_content(const char* markdown_content, const char* query) {
    RelevantContent result = {NULL, NULL, NULL};
    char* content_copy = strdup(markdown_content);
    char* query_copy = strdup(query);
    to_lowercase(content_copy);
    to_lowercase(query_copy);
    
    char* section_start = content_copy;
    char* section_end;
    char* best_section = NULL;
    int best_score = 0;
    
    while ((section_end = strstr(section_start, "\n## ")) != NULL) {
        *section_end = '\0';
        int score = 0;
        char* word = strtok(query_copy, " ");
        while (word != NULL) {
            if (contains_word(section_start, word)) {
                score++;
            }
            word = strtok(NULL, " ");
        }
        if (score > best_score) {
            best_score = score;
            best_section = section_start;
        }
        section_start = section_end + 1;
    }
    
    if (best_section) {
        char* title_end = strchr(best_section, '\n');
        if (title_end) {
            result.title = strndup(best_section, title_end - best_section);
            char* context_start = title_end + 1;
            char* code_start = strstr(context_start, "```");
            if (code_start) {
                result.context = strndup(context_start, code_start - context_start);
                code_start += 3;
                char* code_end = strstr(code_start, "```");
                if (code_end) {
                    result.code = strndup(code_start, code_end - code_start);
                }
            } else {
                result.context = strdup(context_start);
            }
        }
    }
    
    free(content_copy);
    free(query_copy);
    return result;
}

char* format_response(RelevantContent content) {
    if (!content.context && !content.code) {
        return strdup("Désolé, je n'ai pas trouvé d'information pertinente dans la documentation pour répondre à votre question.");
    }
    
    char* response = malloc(1024);
    response[0] = '\0';

    if (content.context) {
        strcat(response, content.context);
        strcat(response, "\n\n");
    }
    
    if (content.code) {
        strcat(response, "Voici un exemple de code pertinent :\n");
        strcat(response, content.code);
    }
    
    return response;
}

int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int matrix[len2 + 1][len1 + 1];

    for (int i = 0; i <= len1; i++) {
        matrix[0][i] = i;
    }
    for (int i = 0; i <= len2; i++) {
        matrix[i][0] = i;
    }

    for (int i = 1; i <= len2; i++) {
        for (int j = 1; j <= len1; j++) {
            int cost = (s1[j - 1] == s2[i - 1]) ? 0 : 1;
            matrix[i][j] = fmin(
                fmin(matrix[i - 1][j] + 1, matrix[i][j - 1] + 1),
                matrix[i - 1][j - 1] + cost
            );
        }
    }

    return matrix[len2][len1];
}

void load_markdown_docs() {
    struct dirent *entry;
    DIR *dp = opendir("./includes/docs/");
    if (dp == NULL) {
        perror("Impossible d'ouvrir le dossier docs");
        return;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) {
            printf("Chargement du fichier: %s\n", entry->d_name);
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "./includes/docs/%s", entry->d_name);
            char *content = read_markdown_file(filepath);

            if (content) {
                docs = realloc(docs, sizeof(MarkdownFile) * (num_docs + 1));
                docs[num_docs].filename = strdup(entry->d_name);
                docs[num_docs].content = content;
                num_docs++;
            }
        }
    }
    closedir(dp);
}

char* read_markdown_file(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Impossible de lire le fichier");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);
    if (content) {
        fread(content, 1, length, file);
        content[length] = '\0';
    }
    fclose(file);

    return content;
}

char* search_in_markdown(const char* content, const char* query) {
    if (content == NULL || query == NULL) {
        return NULL;
    }

    char *found = strcasestr(content, query);
    
    if (found) {
        size_t context_length = 400;
        size_t position = found - content;
        size_t start = position > context_length ? (position - context_length) : 0;
        
        size_t content_length = strlen(content);
        size_t end = (position + context_length) < content_length ? (position + context_length) : content_length;

        char *result = strndup(content + start, end - start);

        if (start > 0 || end < content_length) {
            char *formatted_result = malloc(strlen(result) + 5);
            snprintf(formatted_result, strlen(result) + 5, "%s%s", start > 0 ? "..." : "", result);
            free(result);
            return formatted_result;
        }

        return result;
    }

    return NULL;
}

const char* get_api_key(void) {
    if (!api_key) {
        load_env_variables();
    }
    // printf("API Key: %s\n", api_key);
    return api_key;
}

void free_reflection(Reflection* reflection) {
    free(reflection->thought);
    for (int i = 0; i < reflection->info_count; i++) {
        free(reflection->info[i].content);
    }
    free(reflection->info);
    free(reflection);
}

char* extract_code_from_markdown(const char* content) {
    char* code_start = strstr(content, "```");
    if (code_start == NULL) {
        return NULL;
    }
    
    code_start += 3;
    char* code_end = strstr(code_start, "```");
    if (code_end == NULL) {
        return NULL;
    }
    
    size_t code_length = code_end - code_start;
    char* code = malloc(code_length + 1);
    strncpy(code, code_start, code_length);
    code[code_length] = '\0';
    
    return code;
}

char* find_relevant_code(const char* query) {
    char* best_match = NULL;
    int best_distance = INT_MAX;

    for (int i = 0; i < num_docs; i++) {
        char* code = extract_code_from_markdown(docs[i].content);
        if (code != NULL) {
            int distance = levenshtein_distance(query, code);
            if (distance < best_distance) {
                best_distance = distance;
                free(best_match);
                best_match = code;
            } else {
                free(code);
            }
        }
    }

    return best_match;
}

char* oracle_process(const char* input) {
    RelevantContent best_content = {NULL, NULL, NULL};
    int best_score = 0;
    
    for (int i = 0; i < num_docs; i++) {
        RelevantContent content = extract_relevant_content(docs[i].content, input);
        int score = 0;
        if (content.title) score++;
        if (content.context) score++;
        if (content.code) score++;
        
        if (score > best_score) {
            best_score = score;
            if (best_content.title) free(best_content.title);
            if (best_content.context) free(best_content.context);
            if (best_content.code) free(best_content.code);
            best_content = content;
        } else {
            if (content.title) free(content.title);
            if (content.context) free(content.context);
            if (content.code) free(content.code);
        }
    }
    
    char* response = format_response(best_content);
    if (best_content.title) free(best_content.title);
    if (best_content.context) free(best_content.context);
    if (best_content.code) free(best_content.code);
    return response;
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

        char* escaped_input = curl_easy_escape(curl, input, 0);
        
        char request_body[2048];
        snprintf(request_body, sizeof(request_body),
            "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]}",
            escaped_input);
        
        curl_free(escaped_input);

        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return strdup("Une erreur s'est produite lors de la communication avec l'API.");
        }

        printf("Réponse JSON brute:\n%s\n", chunk.memory);

        struct json_object *parsed_json;
        struct json_object *choices;
        struct json_object *first_choice;
        struct json_object *message;
        struct json_object *content;

        parsed_json = json_tokener_parse(chunk.memory);

        if (parsed_json == NULL) {
            printf("Erreur lors du parsing JSON. La réponse n'est pas un JSON valide.\n");
            free(chunk.memory);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return strdup("Erreur lors du parsing de la réponse de l'API.");
        }

        struct json_object *error_obj;
        if (json_object_object_get_ex(parsed_json, "error", &error_obj)) {
            struct json_object *error_message;
            if (json_object_object_get_ex(error_obj, "message", &error_message)) {
                const char *error_str = json_object_get_string(error_message);
                char *error_response = strdup(error_str);
                json_object_put(parsed_json);
                free(chunk.memory);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return error_response;
            }
        }

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

        printf("Structure JSON inattendue. Impossible de trouver le contenu de la réponse.\n");

        json_object_put(parsed_json);
        free(chunk.memory);
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        return strdup("Erreur lors du parsing de la réponse de l'API.");
    }

    curl_global_cleanup();
    return strdup("Erreur lors de l'initialisation de CURL.");
}

static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp) {
    const char *text;
    (void)handle;
    (void)userp;

    switch (type) {
    case CURLINFO_TEXT:
        fprintf(stderr, "== Info: %s", data);
    default:
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    fprintf(stderr, "%s, %lu bytes (0x%lx)\n", text, (unsigned long)size, (unsigned long)size);
    return 0;
}

char* search_in_markdown_with_similarity(const char* content, const char* query) {
    if (content == NULL || query == NULL) {
        return NULL;
    }

    char *content_lower = strdup(content);
    char *query_lower = strdup(query);
    to_lowercase(content_lower);
    to_lowercase(query_lower);

    int max_levenshtein_distance = 10;
    char *best_match = NULL;
    int best_distance = max_levenshtein_distance;

    char *token = strtok(content_lower, "\n\n");
    while (token != NULL) {
        int distance = levenshtein_distance(token, query_lower);

        if (distance < best_distance) {
            best_distance = distance;

            if (best_match) {
                free(best_match);
            }

            best_match = strdup(token);
        }

        token = strtok(NULL, "\n\n");
    }

    free(content_lower);
    free(query_lower);

    if (best_match != NULL) {
        return best_match;
    }

    return strdup("Je n'ai trouvé aucune information pertinente.");
}
