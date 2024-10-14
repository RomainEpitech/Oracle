#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

void start_new_conversation(const char *conversation_id);
void save_message(const char *role, const char *message, const char *conversation_id);
void load_conversation_history(const char *conversation_id);

#endif