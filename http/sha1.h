/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

#ifndef __SHA1_H
#define __SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} SHA1_CTX;

#define SHA1_DIGEST_SIZE 20

void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context, const uint8_t* data, const unsigned long len);
void SHA1_Final(uint8_t digest[SHA1_DIGEST_SIZE],SHA1_CTX* context);  //WARNINGThe parameters are flipped here. (CNL)

#ifdef __cplusplus
}
#endif

#endif /* __SHA1_H */
