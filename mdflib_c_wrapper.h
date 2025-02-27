#ifndef MDFLIB_C_WRAPPER_H
#define MDFLIB_C_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer type for Mdf4File objects (C doesn't know the class).
typedef void* Mdf4FileHandle;

struct Message {
	uint64_t timestamp;
	uint32_t id;
	uint8_t dlc;
	uint8_t data[8];
	uint8_t channel;
};

// Example functions:
Mdf4FileHandle mdf4_canlog_create(const char* filepath);
int mdf4_canlog_close(Mdf4FileHandle handle, uint64_t tick_time);
int mdf4_canlog_write(Mdf4FileHandle handle, struct Message* message);
int mdf4_canlog_set_meta(Mdf4FileHandle handle, const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif
