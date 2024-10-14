#ifndef ENV_LOADER_H
#define ENV_LOADER_H

int load_env(const char* filename);
const char* get_env(const char* key);

#endif // ENV_LOADER_H