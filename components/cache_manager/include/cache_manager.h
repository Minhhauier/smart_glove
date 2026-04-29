#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

void tts_cache_init(void);                               // gọi sau nvs_flash_init()
uint32_t fnv1a_32(const char *s);
bool tts_cache_load(const char *text);
bool tts_cache_load_by_key(const char *key);              // key tối đa 13 ký tự
bool ensure_mp3_capacity(size_t needed);
void tts_cache_store(const char *text, const uint8_t *data, size_t len);
void tts_cache_store_by_key(const char *key, const uint8_t *data, size_t len); // key tối đa 13 ký tự

#endif