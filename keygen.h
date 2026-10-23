#ifndef KEYGEN_H
#define KEYGEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef uint32_t kg_u32;
typedef int32_t kg_s32;

/* random must be in 1..0xFFFEE, as in the original caller. */
void kg_make_serial(uint32_t random, char serial[17], char authorization[8]);

/* Returns 0 for NULL or lengths other than serial=16, auth=7, registration=6. */
int kg_make_activation(const char *serial, const char *authorization, const char *registration, char activation[16]);

/* gui-compatible: accepts blank/partial serial and authorization buffers.
   the input arrays are fixed-size and may retain data beyond the first nul. */
int kg_make_activation_raw(const char serial[17], const char authorization[8], const char registration[7], char activation[16]);

/* the rng state starts at 0xdeadbeef in the original executable. */
void kg_reset_rng(void);
uint32_t kg_next_random(uint32_t tick, uint32_t upper_bound);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // KEYGEN_H
