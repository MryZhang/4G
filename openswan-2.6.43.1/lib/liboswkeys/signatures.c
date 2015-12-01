/*
 * interfaces to the secrets.c library functions in libopenswan.
 * for now, just stupid wrappers!
 *
 * Copyright (C) 1998-2001  D. Hugh Redelmeier.
 * Copyright (C) 2003-2008  Michael Richardson <mcr@xelerance.com>
 * Copyright (C) 2003-2010 Paul Wouters <paul@xelerance.com>
 * Copyright (C) 2008 David McCullough <david_mccullough@securecomputing.com>
 * Copyright (C) 2009 Avesh Agarwal <avagarwa@redhat.com>
 * Copyright (C) 2010 Tuomo Soini <tis@foobar.fi>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * Modifications to use OCF interface written by
 * Daniel Djamaludin <danield@cyberguard.com>
 * Copyright (C) 2004-2005 Intel Corporation.
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>

#include <glob.h>
#ifndef GLOB_ABORTED
# define GLOB_ABORTED    GLOB_ABEND	/* fix for old versions */
#endif

#include <openswan.h>
#include <openswan/ipsec_policy.h>

#include "sysdep.h"
#include "constants.h"
#include "pluto/defs.h"
#include "id.h"
#include "x509.h"
#include "pgp.h"
#include "certs.h"
#ifdef XAUTH_USEPAM
#include <security/pam_appl.h>
#endif
#include "oswlog.h"
#include "mpzfuncs.h"

#include "oswcrypto.h"

#ifdef HAVE_LIBNSS
 /* nspr */
# include <prerror.h>
# include <prinit.h>
# include <prmem.h>
 /* nss */
# include <key.h>
# include <keyt.h>
# include <nss.h>
# include <pk11pub.h>
# include <seccomon.h>
# include <secerr.h>
# include <secport.h>
# include <time.h>
# include "oswconf.h"
#endif

//struct secret *pluto_secrets = NULL;

#ifdef HAVE_LIBNSS
static int sign_hash_nss(const struct RSA_private_key *k
	, const u_char *hash_val, size_t hash_len
	, u_char *sig_val, size_t sig_len)
{
    SECKEYPrivateKey *privateKey = NULL;
    SECItem signature;
    SECItem data;
    SECItem ckaId;
    PK11SlotInfo *slot = NULL;

    DBG(DBG_CRYPT, DBG_log("RSA_sign_hash: Started using NSS"));

    ckaId.type=siBuffer;
    ckaId.len = k->ckaid_len;
    ckaId.data = DISCARD_CONST(unsigned char *, k->ckaid);

    slot = PK11_GetInternalKeySlot();
    if (slot == NULL) {
	loglog(RC_LOG_SERIOUS, "RSA_sign_hash: Unable to find (slot security) device (err %d)\n", PR_GetError());
	return 0;
    }

    if( PK11_Authenticate(slot, PR_FALSE,osw_return_nss_password_file_info()) == SECSuccess ) {
	DBG(DBG_CRYPT, DBG_log("NSS: Authentication to NSS successful\n"));
    }
    else {
	DBG(DBG_CRYPT, DBG_log("NSS: Authentication to NSS either failed or not required,if NSS DB without password\n"));
    }

    privateKey = PK11_FindKeyByKeyID(slot, &ckaId, osw_return_nss_password_file_info());
    if(privateKey==NULL) {
        DBG(DBG_CRYPT,
            DBG_log("Can't find the private key from the NSS CKA_ID"));
	if(k->pub.nssCert != NULL) {
	   privateKey = PK11_FindKeyByAnyCert(k->pub.nssCert,  osw_return_nss_password_file_info());
           if (privateKey == NULL) {
               loglog(RC_LOG_SERIOUS,
                      "Can't find the private key from the NSS CERT (err %d)",
                      PR_GetError());
           }
	}
    }

    PK11_FreeSlot(slot);

    if (privateKey == NULL) {
	loglog(RC_LOG_SERIOUS, "Can't find the private key from the NSS CERT (err %d)\n", PR_GetError());
	return 0;
    }

    data.type=siBuffer;
    data.len=hash_len;
    data.data = DISCARD_CONST(u_char *, hash_val);

    /*signature.len=PK11_SignatureLen(privateKey);*/
    signature.len=sig_len;
    signature.data=sig_val;

    {
        SECStatus s = PK11_Sign(privateKey, &signature, &data);

        if (s != SECSuccess) {
            loglog(RC_LOG_SERIOUS,
                   "RSA_sign_hash: sign function failed (%d)",
                   PR_GetError());
            return 0;
        }
    }

    DBG(DBG_CRYPT, DBG_log("RSA_sign_hash: Ended using NSS"));
    return signature.len;
}
#endif

/*
 * compute an RSA signature with PKCS#1 padding
 */
void
sign_hash(const struct RSA_private_key *k
	  , const u_char *hash_val, size_t hash_len
	  , u_char *sig_val, size_t sig_len)
{
#ifdef HAVE_LIBNSS
    sign_hash_nss(k,hash_val,hash_len,sig_val,sig_len);
#else
    chunk_t ch;
    mpz_t t1;
    size_t padlen;
    u_char *p = sig_val;

    DBG(DBG_CONTROL | DBG_CRYPT,
	DBG_log("signing hash with RSA Key *%s", k->pub.keyid)
    )
    /* PKCS#1 v1.5 8.1 encryption-block formatting */
    *p++ = 0x00;
    *p++ = 0x01;	/* BT (block type) 01 */
    padlen = sig_len - 3 - hash_len;
    memset(p, 0xFF, padlen);
    p += padlen;
    *p++ = 0x00;
    memcpy(p, hash_val, hash_len);
    passert(p + hash_len - sig_val == (ptrdiff_t)sig_len);

    /* PKCS#1 v1.5 8.2 octet-string-to-integer conversion */
    n_to_mpz(t1, sig_val, sig_len);	/* (could skip leading 0x00) */

    /* PKCS#1 v1.5 8.3 RSA computation y = x^c mod n
     * Better described in PKCS#1 v2.0 5.1 RSADP.
     * There are two methods, depending on the form of the private key.
     * We use the one based on the Chinese Remainder Theorem.
     */
    oswcrypto.rsa_mod_exp_crt(t1, t1, &k->p, &k->dP, &k->q, &k->dQ, &k->qInv);
    /* PKCS#1 v1.5 8.4 integer-to-octet-string conversion */
    ch = mpz_to_n(t1, sig_len);
    memcpy(sig_val, ch.ptr, sig_len);
    pfree(ch.ptr);

    mpz_clear(t1);
#endif
}

#ifdef HAVE_LIBNSS
err_t RSA_signature_verify_nss(const struct RSA_public_key *k
                              , const u_char *hash_val, size_t hash_len
                               ,const u_char *sig_val, size_t sig_len)
{
   SECKEYPublicKey *publicKey;
   PRArenaPool *arena;
   SECStatus retVal = SECSuccess;
   SECItem nss_n, nss_e;
   SECItem signature, data;
   chunk_t n,e;

    /*Converting n and e to form public key in SECKEYPublicKey data structure*/

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return "10" "NSS error: Not enough memory to create arena";
    }

    publicKey = (SECKEYPublicKey *) PORT_ArenaZAlloc (arena, sizeof (SECKEYPublicKey));
    if (!publicKey) {
	PORT_FreeArena (arena, PR_FALSE);
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return "11" "NSS error: Not enough memory to create publicKey";
    }

    publicKey->arena = arena;
    publicKey->keyType = rsaKey;
    publicKey->pkcs11Slot = NULL;
    publicKey->pkcs11ID = CK_INVALID_HANDLE;

    /* Converting n(modulus) and e(exponent) from mpz_t form to chunk_t */
    n = mpz_to_n_autosize(&k->n);
    e = mpz_to_n_autosize(&k->e);

    /*Converting n and e to nss_n and nss_e*/
    nss_n.data = n.ptr;
    nss_n.len = (unsigned int)n.len;
    nss_n.type = siBuffer;

    nss_e.data = e.ptr;
    nss_e.len = (unsigned int)e.len;
    nss_e.type = siBuffer;

    retVal = SECITEM_CopyItem(arena, &publicKey->u.rsa.modulus, &nss_n);
    if (retVal == SECSuccess) {
	retVal = SECITEM_CopyItem (arena, &publicKey->u.rsa.publicExponent, &nss_e);
    }

    if(retVal != SECSuccess) {
	pfree(n.ptr);
	pfree(e.ptr);
	SECKEY_DestroyPublicKey (publicKey);
	return "12" "NSS error: Not able to copy modulus or exponent or both while forming SECKEYPublicKey structure";
    }
    signature.type = siBuffer;
    signature.data = DISCARD_CONST(unsigned char *, sig_val);
    signature.len  = (unsigned int)sig_len;

    data.len = (unsigned int)sig_len;
    data.data = alloc_bytes(data.len, "NSS decrypted signature");
    data.type = siBuffer;

    if(PK11_VerifyRecover(publicKey, &signature, &data, osw_return_nss_password_file_info()) == SECSuccess ) {
	DBG(DBG_CRYPT,DBG_dump("NSS RSA verify: decrypted sig: ", data.data, data.len));
    }
    else {
        DBG(DBG_CRYPT,DBG_log("NSS RSA verify: decrypting signature is failed"));
        return "13" "NSS error: Not able to decrypt";
    }

    if(memcmp(data.data+data.len-hash_len, hash_val, hash_len)!=0) {
	pfree(data.data);
	loglog(RC_LOG_SERIOUS, "RSA Signature NOT verified");
	return "14" "NSS error: Not able to verify";
    }

    DBG(DBG_CRYPT,DBG_dump("NSS RSA verify: hash value: ", hash_val, hash_len));

    pfree(data.data);
    pfree(n.ptr);
    pfree(e.ptr);
    SECKEY_DestroyPublicKey (publicKey);

    DBG(DBG_CRYPT, DBG_log("RSA Signature verified"));

    return NULL;
}
#endif

/*
 * Local Variables:
 * c-basic-offset:4
 * c-style: pluto
 * End:
 */
