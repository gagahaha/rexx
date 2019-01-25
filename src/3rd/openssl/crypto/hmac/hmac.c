
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cryptlib.h"
#include <openssl/hmac.h>

#ifdef OPENSSL_FIPS
# include <openssl/fips.h>
#endif

int HMAC_Init_ex(HMAC_CTX *ctx, const void *key, int len,
                 const EVP_MD *md, ENGINE *impl)
{
    int i, j, reset = 0;
    unsigned char pad[HMAC_MAX_MD_CBLOCK];

#ifdef OPENSSL_FIPS
    /* If FIPS mode switch to approved implementation if possible */
    if (FIPS_mode()) {
        const EVP_MD *fipsmd;
        if (md) {
            fipsmd = FIPS_get_digestbynid(EVP_MD_type(md));
            if (fipsmd)
                md = fipsmd;
        }
    }

    if (FIPS_mode()) {
        /* If we have an ENGINE need to allow non FIPS */
        if ((impl || ctx->i_ctx.engine)
            && !(ctx->i_ctx.flags & EVP_CIPH_FLAG_NON_FIPS_ALLOW)) {
            EVPerr(EVP_F_HMAC_INIT_EX, EVP_R_DISABLED_FOR_FIPS);
            return 0;
        }
        /*
         * Other algorithm blocking will be done in FIPS_cmac_init, via
         * FIPS_hmac_init_ex().
         */
        if (!impl && !ctx->i_ctx.engine)
            return FIPS_hmac_init_ex(ctx, key, len, md, NULL);
    }
#endif
    /* If we are changing MD then we must have a key */
    if (md != NULL && md != ctx->md && (key == NULL || len < 0))
        return 0;

    if (md != NULL) {
        reset = 1;
        ctx->md = md;
    } else if (ctx->md) {
        md = ctx->md;
    } else {
        return 0;
    }

    if (key != NULL) {
        reset = 1;
        j = EVP_MD_block_size(md);
        OPENSSL_assert(j <= (int)sizeof(ctx->key));
        if (j < len) {
            if (!EVP_DigestInit_ex(&ctx->md_ctx, md, impl))
                goto err;
            if (!EVP_DigestUpdate(&ctx->md_ctx, key, len))
                goto err;
            if (!EVP_DigestFinal_ex(&(ctx->md_ctx), ctx->key,
                                    &ctx->key_length))
                goto err;
        } else {
            if (len < 0 || len > (int)sizeof(ctx->key))
                return 0;
            memcpy(ctx->key, key, len);
            ctx->key_length = len;
        }
        if (ctx->key_length != HMAC_MAX_MD_CBLOCK)
            memset(&ctx->key[ctx->key_length], 0,
                   HMAC_MAX_MD_CBLOCK - ctx->key_length);
    }

    if (reset) {
        for (i = 0; i < HMAC_MAX_MD_CBLOCK; i++)
            pad[i] = 0x36 ^ ctx->key[i];
        if (!EVP_DigestInit_ex(&ctx->i_ctx, md, impl))
            goto err;
        if (!EVP_DigestUpdate(&ctx->i_ctx, pad, EVP_MD_block_size(md)))
            goto err;

        for (i = 0; i < HMAC_MAX_MD_CBLOCK; i++)
            pad[i] = 0x5c ^ ctx->key[i];
        if (!EVP_DigestInit_ex(&ctx->o_ctx, md, impl))
            goto err;
        if (!EVP_DigestUpdate(&ctx->o_ctx, pad, EVP_MD_block_size(md)))
            goto err;
    }
    if (!EVP_MD_CTX_copy_ex(&ctx->md_ctx, &ctx->i_ctx))
        goto err;
    return 1;
 err:
    return 0;
}

int HMAC_Init(HMAC_CTX *ctx, const void *key, int len, const EVP_MD *md)
{
    if (key && md)
        HMAC_CTX_init(ctx);
    return HMAC_Init_ex(ctx, key, len, md, NULL);
}

int HMAC_Update(HMAC_CTX *ctx, const unsigned char *data, size_t len)
{
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine)
        return FIPS_hmac_update(ctx, data, len);
#endif
    if (!ctx->md)
        return 0;

    return EVP_DigestUpdate(&ctx->md_ctx, data, len);
}

int HMAC_Final(HMAC_CTX *ctx, unsigned char *md, unsigned int *len)
{
    unsigned int i;
    unsigned char buf[EVP_MAX_MD_SIZE];
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine)
        return FIPS_hmac_final(ctx, md, len);
#endif

    if (!ctx->md)
        goto err;

    if (!EVP_DigestFinal_ex(&ctx->md_ctx, buf, &i))
        goto err;
    if (!EVP_MD_CTX_copy_ex(&ctx->md_ctx, &ctx->o_ctx))
        goto err;
    if (!EVP_DigestUpdate(&ctx->md_ctx, buf, i))
        goto err;
    if (!EVP_DigestFinal_ex(&ctx->md_ctx, md, len))
        goto err;
    return 1;
 err:
    return 0;
}

void HMAC_CTX_init(HMAC_CTX *ctx)
{
    EVP_MD_CTX_init(&ctx->i_ctx);
    EVP_MD_CTX_init(&ctx->o_ctx);
    EVP_MD_CTX_init(&ctx->md_ctx);
    ctx->md = NULL;
}

int HMAC_CTX_copy(HMAC_CTX *dctx, HMAC_CTX *sctx)
{
    if (!EVP_MD_CTX_copy(&dctx->i_ctx, &sctx->i_ctx))
        goto err;
    if (!EVP_MD_CTX_copy(&dctx->o_ctx, &sctx->o_ctx))
        goto err;
    if (!EVP_MD_CTX_copy(&dctx->md_ctx, &sctx->md_ctx))
        goto err;
    memcpy(dctx->key, sctx->key, HMAC_MAX_MD_CBLOCK);
    dctx->key_length = sctx->key_length;
    dctx->md = sctx->md;
    return 1;
 err:
    return 0;
}

void HMAC_CTX_cleanup(HMAC_CTX *ctx)
{
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine) {
        FIPS_hmac_ctx_cleanup(ctx);
        return;
    }
#endif
    EVP_MD_CTX_cleanup(&ctx->i_ctx);
    EVP_MD_CTX_cleanup(&ctx->o_ctx);
    EVP_MD_CTX_cleanup(&ctx->md_ctx);
    OPENSSL_cleanse(ctx, sizeof *ctx);
}

unsigned char *HMAC(const EVP_MD *evp_md, const void *key, int key_len,
                    const unsigned char *d, size_t n, unsigned char *md,
                    unsigned int *md_len)
{
    HMAC_CTX c;
    static unsigned char m[EVP_MAX_MD_SIZE];

    if (md == NULL)
        md = m;
    HMAC_CTX_init(&c);
    if (!HMAC_Init(&c, key, key_len, evp_md))
        goto err;
    if (!HMAC_Update(&c, d, n))
        goto err;
    if (!HMAC_Final(&c, md, md_len))
        goto err;
    HMAC_CTX_cleanup(&c);
    return md;
 err:
    HMAC_CTX_cleanup(&c);
    return NULL;
}

void HMAC_CTX_set_flags(HMAC_CTX *ctx, unsigned long flags)
{
    EVP_MD_CTX_set_flags(&ctx->i_ctx, flags);
    EVP_MD_CTX_set_flags(&ctx->o_ctx, flags);
    EVP_MD_CTX_set_flags(&ctx->md_ctx, flags);
}