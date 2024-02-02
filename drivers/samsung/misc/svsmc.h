/*
 * Copyright (C) 2020 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/* SMC ID */
#define SMC_CM_SSS_HASH			(0xC2001027)
#define SMC_CM_SECURE_BOOT		(0xC200101D)

/* SMC CMD */
#define CMD_HASH_NWD_IMG		(2)
#define CMD_CHECK_SIGN_NWD		(7)
#define CMD_CHECK_SIGN_NWD_WITH_HASH	(21)

/* Crypto variable */
#define SHA_HASH_MAX_LEN		(128)
#define SHA_HASH_MAX_DIGEST_LEN		(64)
#define SHA_HMAC_MAX_KEY_LEN		(128)
#define WORD_SIZE_IN_BYTE		(4)
#define SHA_DIGEST_LEN			(64)

enum sha_alg_t {
	ALG_SHA1,
	ALG_SHA256,
	ALG_SHA512,
};

enum sha_func_t {
	SHA_INIT,
	SHA_UPDATE,
	SHA_FINAL,
	SHA_DIGEST,
};

typedef struct _hash_ctx {
	uint32_t alg;
	uint32_t key_len;
	uint32_t key[SHA_HMAC_MAX_KEY_LEN / WORD_SIZE_IN_BYTE];
	uint32_t buflen;
	unsigned char buffer[SHA_HASH_MAX_LEN];
	uint32_t state[SHA_HASH_MAX_DIGEST_LEN / WORD_SIZE_IN_BYTE];
	uint32_t prelen_high;
	uint32_t prelen_low;
} hash_ctx;

typedef struct _sha_ctx {
	enum sha_func_t func;
	/* init */
	enum sha_alg_t alg;
	/* update */
	uint64_t input;
	uint64_t input_len;
	/* final */
	uint64_t output;
	uint64_t output_len;
	/* init/update/final */
	hash_ctx ictx;
} sha_ctx;

typedef struct _sb_ctx {
	uint32_t sign_type;
	uint32_t key_type;
	uint32_t key_index;
	uint32_t reserved_32[3];
	uint64_t pub_key_ptr;
	uint64_t pub_key_len;
	uint64_t signed_img_ptr;
	uint64_t signed_img_len;
	uint64_t sign_field_ptr;
	uint64_t sign_field_len;
	uint8_t hash[SHA_DIGEST_LEN];
	uint64_t hash_len;
	uint64_t reserved_64[6];
} sb_ctx;

typedef struct _smc_ioctl_info {
	unsigned int offset;
	unsigned int data_len;
} smc_ioctl_info;

