/**
 * @file crypto_config.h
 * @brief Minimal CycloneCRYPTO config for bootloader (CRC32 + SHA256 only)
 */

#ifndef _CRYPTO_CONFIG_H
#define _CRYPTO_CONFIG_H

#define GPL_LICENSE_TERMS_ACCEPTED

// Only what the bootloader needs for image integrity verification
#define CRYPTO_STATIC_MEM_SUPPORT  DISABLED
#define MPI_SUPPORT                DISABLED
#define MPI_ASM_SUPPORT            DISABLED

// Hash: SHA256 for image verification
// SHA1/224/384/512 enabled because CycloneBOOT v2.6.2 boot_common.c
// references all hash algo symbols in validation checks
#define SHA256_SUPPORT             ENABLED
#define MD5_SUPPORT                DISABLED
#define SHA1_SUPPORT               ENABLED
#define SHA224_SUPPORT             ENABLED
#define SHA384_SUPPORT             ENABLED
#define SHA512_SUPPORT             ENABLED

// MAC: HMAC for future use
#define HMAC_SUPPORT               ENABLED
#define CMAC_SUPPORT               DISABLED
#define GMAC_SUPPORT               DISABLED

// Everything else disabled
#define AES_SUPPORT                DISABLED
#define DES_SUPPORT                DISABLED
#define DES3_SUPPORT               DISABLED
#define RSA_SUPPORT                DISABLED
#define EC_SUPPORT                 DISABLED
#define ECDSA_SUPPORT              DISABLED
#define ECDH_SUPPORT               DISABLED
#define X509_SUPPORT               DISABLED
#define YARROW_SUPPORT             DISABLED
#define TRNG_SUPPORT               DISABLED

// Encoding
#define BASE64_SUPPORT             DISABLED
#define ASN1_SUPPORT               DISABLED
#define OID_SUPPORT                DISABLED
#define PEM_SUPPORT                DISABLED

// Cipher modes
#define ECB_SUPPORT                DISABLED
#define CBC_SUPPORT                DISABLED
#define CTR_SUPPORT                DISABLED
#define GCM_SUPPORT                DISABLED
#define CCM_SUPPORT                DISABLED

#endif
