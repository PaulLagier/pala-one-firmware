#pragma once
#include <openssl/evp.h>
struct mbedtls_sha256_context {EVP_MD_CTX* inner=nullptr;};
inline void mbedtls_sha256_init(mbedtls_sha256_context* c){c->inner=nullptr;}
inline void mbedtls_sha256_free(mbedtls_sha256_context* c){EVP_MD_CTX_free(c->inner);c->inner=nullptr;}
inline int mbedtls_sha256_starts(mbedtls_sha256_context* c,int){
  c->inner=EVP_MD_CTX_new();return c->inner && EVP_DigestInit_ex(c->inner,EVP_sha256(),nullptr)==1?0:-1;
}
inline int mbedtls_sha256_update(mbedtls_sha256_context* c,const uint8_t* data,size_t n){return EVP_DigestUpdate(c->inner,data,n)==1?0:-1;}
inline int mbedtls_sha256_finish(mbedtls_sha256_context* c,uint8_t* out){unsigned n=0;return EVP_DigestFinal_ex(c->inner,out,&n)==1 && n==32?0:-1;}
