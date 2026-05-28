/**
 * @file crypto_config.h
 * @brief CycloneCRYPTO configuration file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2021-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.4
 **/

#ifndef _CRYPTO_CONFIG_H
#define _CRYPTO_CONFIG_H

//-----------------------------------------------------------------------------
// GPL License Acceptance (required for open source version)
//-----------------------------------------------------------------------------
#define GPL_LICENSE_TERMS_ACCEPTED

//-----------------------------------------------------------------------------
// General Configuration
//-----------------------------------------------------------------------------
// Disable static memory allocation (use dynamic)
#define CRYPTO_STATIC_MEM_SUPPORT  DISABLED

//-----------------------------------------------------------------------------
// Multiple Precision Integer (MPI) Support
//-----------------------------------------------------------------------------
#define MPI_SUPPORT                ENABLED
#define MPI_ASM_SUPPORT            DISABLED

//-----------------------------------------------------------------------------
// Encoding Support
//-----------------------------------------------------------------------------
#define BASE64_SUPPORT             ENABLED
#define BASE64URL_SUPPORT          ENABLED
#define RADIX64_SUPPORT            DISABLED
#define ASN1_SUPPORT               ENABLED
#define OID_SUPPORT                ENABLED
#define PEM_SUPPORT                ENABLED

//-----------------------------------------------------------------------------
// Hash Algorithm Support
// Enable algorithms needed for firmware verification
//-----------------------------------------------------------------------------
// Legacy hash algorithms (disabled for security)
#define MD2_SUPPORT                DISABLED
#define MD4_SUPPORT                DISABLED
#define MD5_SUPPORT                DISABLED
#define RIPEMD128_SUPPORT          DISABLED
#define RIPEMD160_SUPPORT          DISABLED

// SHA-1 (legacy, consider disabling in production)
#define SHA1_SUPPORT               DISABLED

// SHA-2 family (recommended)
#define SHA224_SUPPORT             DISABLED
#define SHA256_SUPPORT             ENABLED    // Primary hash for integrity
#define SHA384_SUPPORT             DISABLED
#define SHA512_SUPPORT             DISABLED
#define SHA512_224_SUPPORT         DISABLED
#define SHA512_256_SUPPORT         DISABLED

// SHA-3 family (optional)
#define SHA3_224_SUPPORT           DISABLED
#define SHA3_256_SUPPORT           DISABLED
#define SHA3_384_SUPPORT           DISABLED
#define SHA3_512_SUPPORT           DISABLED
#define SHAKE128_SUPPORT           DISABLED
#define SHAKE256_SUPPORT           DISABLED

// Other hash algorithms
#define KECCAK_SUPPORT             DISABLED
#define BLAKE2B_SUPPORT            DISABLED
#define BLAKE2B160_SUPPORT         DISABLED
#define BLAKE2B256_SUPPORT         DISABLED
#define BLAKE2B384_SUPPORT         DISABLED
#define BLAKE2B512_SUPPORT         DISABLED
#define BLAKE2S_SUPPORT            DISABLED
#define BLAKE2S128_SUPPORT         DISABLED
#define BLAKE2S160_SUPPORT         DISABLED
#define BLAKE2S224_SUPPORT         DISABLED
#define BLAKE2S256_SUPPORT         DISABLED
#define TIGER_SUPPORT              DISABLED
#define WHIRLPOOL_SUPPORT          DISABLED
#define SM3_SUPPORT                DISABLED

//-----------------------------------------------------------------------------
// MAC Algorithm Support
//-----------------------------------------------------------------------------
#define HMAC_SUPPORT               ENABLED    // For HMAC-based authentication
#define GMAC_SUPPORT               DISABLED
#define CMAC_SUPPORT               DISABLED
#define KMAC_SUPPORT               DISABLED
#define POLY1305_SUPPORT           DISABLED

//-----------------------------------------------------------------------------
// Symmetric Cipher Support
// Enable if using encrypted firmware images
//-----------------------------------------------------------------------------
// Block ciphers
#define AES_SUPPORT                ENABLED    // Primary cipher
#define DES_SUPPORT                DISABLED
#define DES3_SUPPORT               DISABLED
#define IDEA_SUPPORT               DISABLED
#define BLOWFISH_SUPPORT           DISABLED
#define CAMELLIA_SUPPORT           DISABLED
#define ARIA_SUPPORT               DISABLED
#define SEED_SUPPORT               DISABLED
#define SM4_SUPPORT                DISABLED
#define PRESENT_SUPPORT            DISABLED
#define TEA_SUPPORT                DISABLED
#define XTEA_SUPPORT               DISABLED
#define TRIVIUM_SUPPORT            DISABLED
#define ZUC_SUPPORT                DISABLED

// Stream ciphers
#define RC4_SUPPORT                DISABLED
#define CHACHA_SUPPORT             DISABLED
#define SALSA20_SUPPORT            DISABLED

// Cipher modes
#define ECB_SUPPORT                DISABLED
#define CBC_SUPPORT                ENABLED
#define CTR_SUPPORT                ENABLED
#define CFB_SUPPORT                DISABLED
#define OFB_SUPPORT                DISABLED
#define XTS_SUPPORT                DISABLED
#define CCM_SUPPORT                DISABLED
#define GCM_SUPPORT                ENABLED    // For authenticated encryption

//-----------------------------------------------------------------------------
// Public Key Cryptography Support
// Enable if using signature verification
//-----------------------------------------------------------------------------
// RSA
#define RSA_SUPPORT                DISABLED
#define RSA_PUBLIC_KEY_SUPPORT     DISABLED
#define RSA_PRIVATE_KEY_SUPPORT    DISABLED

// DSA
#define DSA_SUPPORT                DISABLED
#define DSA_PUBLIC_KEY_SUPPORT     DISABLED
#define DSA_PRIVATE_KEY_SUPPORT    DISABLED

// Elliptic Curve Cryptography
#define EC_SUPPORT                 DISABLED
#define ECDH_SUPPORT               DISABLED
#define ECDSA_SUPPORT              DISABLED

// Curve support (enable if using ECDSA)
#define SECP160K1_SUPPORT          DISABLED
#define SECP160R1_SUPPORT          DISABLED
#define SECP160R2_SUPPORT          DISABLED
#define SECP192K1_SUPPORT          DISABLED
#define SECP192R1_SUPPORT          DISABLED
#define SECP224K1_SUPPORT          DISABLED
#define SECP224R1_SUPPORT          DISABLED
#define SECP256K1_SUPPORT          DISABLED
#define SECP256R1_SUPPORT          DISABLED   // P-256 - enable for ECDSA
#define SECP384R1_SUPPORT          DISABLED   // P-384 - enable for ECDSA
#define SECP521R1_SUPPORT          DISABLED
#define BRAINPOOLP256R1_SUPPORT    DISABLED
#define BRAINPOOLP384R1_SUPPORT    DISABLED
#define BRAINPOOLP512R1_SUPPORT    DISABLED
#define SM2_SUPPORT                DISABLED
#define X25519_SUPPORT             DISABLED
#define X448_SUPPORT               DISABLED
#define ED25519_SUPPORT            DISABLED
#define ED448_SUPPORT              DISABLED

//-----------------------------------------------------------------------------
// Key Derivation Function Support
//-----------------------------------------------------------------------------
#define HKDF_SUPPORT               DISABLED
#define PBKDF_SUPPORT              DISABLED
#define CONCAT_KDF_SUPPORT         DISABLED
#define BCRYPT_SUPPORT             DISABLED
#define SCRYPT_SUPPORT             DISABLED
#define ARGON2_SUPPORT             DISABLED

//-----------------------------------------------------------------------------
// Random Number Generator Support
//-----------------------------------------------------------------------------
#define YARROW_SUPPORT             ENABLED
#define TRNG_SUPPORT               ENABLED    // Use hardware TRNG if available

//-----------------------------------------------------------------------------
// X.509 Certificate Support (for signature verification)
//-----------------------------------------------------------------------------
#define X509_SUPPORT               DISABLED
#define X509_CERT_PARSE_SUPPORT    DISABLED
#define X509_CERT_VALIDATE_SUPPORT DISABLED
#define X509_CRL_PARSE_SUPPORT     DISABLED
#define X509_CRL_VALIDATE_SUPPORT  DISABLED
#define X509_CSR_PARSE_SUPPORT     DISABLED
#define X509_CSR_CREATE_SUPPORT    DISABLED
#define X509_SIGN_SUPPORT          DISABLED

//-----------------------------------------------------------------------------
// Hardware Acceleration (STM32H5xx)
// Enable to use hardware crypto accelerator
//-----------------------------------------------------------------------------
// Uncomment to enable STM32H5xx hardware acceleration
// #define STM32H5XX_CRYPTO_HASH_SUPPORT    ENABLED
// #define STM32H5XX_CRYPTO_CIPHER_SUPPORT  ENABLED
// #define STM32H5XX_CRYPTO_PKC_SUPPORT     ENABLED
// #define STM32H5XX_CRYPTO_TRNG_SUPPORT    ENABLED

#endif // _CRYPTO_CONFIG_H
