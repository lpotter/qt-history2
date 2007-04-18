/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, Trolltech gives permission to link
** the code of its release of Qt with the OpenSSL project's "OpenSSL" library
** (or modified versions of the "OpenSSL" library that use the same license
** as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#ifndef QSSLSOCKET_OPENSSL_SYMBOLS_P_H
#define QSSLSOCKET_OPENSSL_SYMBOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qsslsocket_openssl_p.h"

#ifdef QT_SHARED
// **************** Shared declarations ******************
// ret func(arg)
#  define DEFINEFUNC(ret, func, arg, a, err)				\
    typedef ret (*_q_PTR_##func)(arg);					\
    static _q_PTR_##func _q_##func = 0;					\
    ret q_##func(arg) {						\
        if (_q_##func) return _q_##func(a);				\
        qWarning("QSslSocket: cannot call unresolved function "#func);	\
        err;								\
    }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2);         \
    static _q_PTR_##func _q_##func = 0;               \
    ret q_##func(arg1, arg2) { \
        if (_q_##func) return _q_##func(a, b); \
        qWarning("QSslSocket: cannot call unresolved function "#func);\
        err; \
    }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3);            \
    static _q_PTR_##func _q_##func = 0;                        \
    ret q_##func(arg1, arg2, arg3) { \
        if (_q_##func) return _q_##func(a, b, c); \
        qWarning("QSslSocket: cannot call unresolved function "#func); \
        err; \
    }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4);               \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4) { \
         if (_q_##func) return _q_##func(a, b, c, d); \
         qWarning("QSslSocket: cannot call unresolved function "#func); \
         err; \
    }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5);         \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4, arg5) { \
        if (_q_##func) return _q_##func(a, b, c, d, e); \
        qWarning("QSslSocket: cannot call unresolved function "#func); \
        err; \
    }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6);   \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { \
        if (_q_##func) return _q_##func(a, b, c, d, e, f); \
        qWarning("QSslSocket: cannot call unresolved function "#func); \
        err; \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
    static _q_PTR_##func _q_##func = 0;                                       \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { \
        if (_q_##func) return _q_##func(a, b, c, d, e, f, g); \
        qWarning("QSslSocket: cannot call unresolved function "#func); \
        err; \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);   \
    static _q_PTR_##func _q_##func = 0;                                                   \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { \
        if (_q_##func) return _q_##func(a, b, c, d, e, f, g, h, i); \
        qWarning("QSslSocket: cannot call unresolved function "#func); \
        err; \
    }
// **************** Shared declarations ******************

#else // !defined QT_SHARED

// **************** Static declarations ******************

// ret func(arg)
#  define DEFINEFUNC(ret, func, arg, a, err)				\
    ret q_##func(arg) {	return func(a); }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err) \
    ret q_##func(arg1, arg2) { return func(a, b); }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err) \
    ret q_##func(arg1, arg2, arg3) { return func(a, b, c); }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err) \
    ret q_##func(arg1, arg2, arg3, arg4) { return func(a, b, c, d); }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5) { return func(a, b, c, d, e); }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { return func(a, b, c, d, e, f); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { return func(a, b, c, d, e, f, g); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { return func(a, b, c, d, e, f, g, h, i); }

// **************** Static declarations ******************

#endif // !defined QT_SHARED

bool q_resolveOpenSslSymbols();
long q_BIO_ctrl(BIO *a, int b, long c, void *d);
int q_BIO_free(BIO *a);
BIO *q_BIO_new(BIO_METHOD *a);
BIO *q_BIO_new_mem_buf(void *a, int b);
int q_BIO_read(BIO *a, void *b, int c);
BIO_METHOD *q_BIO_s_mem();
int q_BIO_write(BIO *a, const void *b, int c);
int q_BN_num_bits(const BIGNUM *a);
int q_CRYPTO_num_locks();
void q_CRYPTO_set_locking_callback(void (*a)(int, int, const char *, int));
void q_CRYPTO_set_id_callback(unsigned long (*a)());
void q_CRYPTO_free(void *a);
void q_DSA_free(DSA *a);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
X509 *q_d2i_X509(X509 **a, const unsigned char **b, long c);
#else
X509 *q_d2i_X509(X509 **a, unsigned char **b, long c);
#endif
char *q_ERR_error_string(unsigned long a, char *b);
unsigned long q_ERR_get_error();
const EVP_CIPHER *q_EVP_des_ede3_cbc();
int q_EVP_PKEY_assign(EVP_PKEY *a, int b, char *c);
void q_EVP_PKEY_free(EVP_PKEY *a);
RSA *q_EVP_PKEY_get1_RSA(EVP_PKEY *a);
DSA *q_EVP_PKEY_get1_DSA(EVP_PKEY *a);
int q_EVP_PKEY_type(int a);
EVP_PKEY *q_EVP_PKEY_new();
int q_i2d_X509(X509 *a, unsigned char **b);
#ifdef SSLEAY_MACROS
// ### verify
void *q_PEM_ASN1_read_bio(d2i_of_void *a, const char *b, BIO *c, void **d, pem_password_cb *e,
                          void *f);
// ### ditto for write
#else
DSA *q_PEM_read_bio_DSAPrivateKey(BIO *a, DSA **b, pem_password_cb *c, void *d);
RSA *q_PEM_read_bio_RSAPrivateKey(BIO *a, RSA **b, pem_password_cb *c, void *d);
int q_PEM_write_bio_DSAPrivateKey(BIO *a, DSA *b, const EVP_CIPHER *c, unsigned char *d,
                                  int e, pem_password_cb *f, void *g);
int q_PEM_write_bio_RSAPrivateKey(BIO *a, RSA *b, const EVP_CIPHER *c, unsigned char *d,
                                  int e, pem_password_cb *f, void *g);
#endif
DSA *q_PEM_read_bio_DSA_PUBKEY(BIO *a, DSA **b, pem_password_cb *c, void *d);
RSA *q_PEM_read_bio_RSA_PUBKEY(BIO *a, RSA **b, pem_password_cb *c, void *d);
int q_PEM_write_bio_DSA_PUBKEY(BIO *a, DSA *b);
int q_PEM_write_bio_RSA_PUBKEY(BIO *a, RSA *b);
void q_RAND_seed(const void *a, int b);
int q_RAND_status();
void q_RSA_free(RSA *a);
int q_sk_num(STACK *a);
char * q_sk_value(STACK *a, int b);
int q_SSL_accept(SSL *a);
int q_SSL_clear(SSL *a);
char *q_SSL_CIPHER_description(SSL_CIPHER *a, char *b, int c);
int q_SSL_connect(SSL *a);
int q_SSL_CTX_check_private_key(const SSL_CTX *a);
long q_SSL_CTX_ctrl(SSL_CTX *a, int b, long c, void *d);
void q_SSL_CTX_free(SSL_CTX *a);
SSL_CTX *q_SSL_CTX_new(SSL_METHOD *a);
int q_SSL_CTX_set_cipher_list(SSL_CTX *a, const char *b);
int q_SSL_CTX_set_default_verify_paths(SSL_CTX *a);
int q_SSL_CTX_use_certificate(SSL_CTX *a, X509 *b);
int q_SSL_CTX_use_certificate_file(SSL_CTX *a, const char *b, int c);
int q_SSL_CTX_use_PrivateKey(SSL_CTX *a, EVP_PKEY *b);
int q_SSL_CTX_use_RSAPrivateKey(SSL_CTX *a, RSA *b);
int q_SSL_CTX_use_PrivateKey_file(SSL_CTX *a, const char *b, int c);
void q_SSL_free(SSL *a);
STACK_OF(SSL_CIPHER) *q_SSL_get_ciphers(const SSL *a);
SSL_CIPHER *q_SSL_get_current_cipher(SSL *a);
int q_SSL_get_error(SSL *a, int b);
STACK_OF(X509) *q_SSL_get_peer_cert_chain(SSL *a);
X509 *q_SSL_get_peer_certificate(SSL *a);
long q_SSL_get_verify_result(const SSL *a);
int q_SSL_library_init();
void q_SSL_load_error_strings();
SSL *q_SSL_new(SSL_CTX *a);
int q_SSL_read(SSL *a, void *b, int c);
void q_SSL_set_bio(SSL *a, BIO *b, BIO *c);
void q_SSL_set_accept_state(SSL *a);
void q_SSL_set_connect_state(SSL *a);
int q_SSL_shutdown(SSL *a);
SSL_METHOD *q_SSLv2_client_method();
SSL_METHOD *q_SSLv3_client_method();
SSL_METHOD *q_SSLv23_client_method();
SSL_METHOD *q_TLSv1_client_method();
SSL_METHOD *q_SSLv2_server_method();
SSL_METHOD *q_SSLv3_server_method();
SSL_METHOD *q_SSLv23_server_method();
SSL_METHOD *q_TLSv1_server_method();
int q_SSL_write(SSL *a, const void *b, int c);
int q_X509_cmp(X509 *a, X509 *b);
X509 *q_X509_dup(X509 *a);
void q_X509_free(X509 *a);
X509_NAME *q_X509_get_issuer_name(X509 *a);
X509_NAME *q_X509_get_subject_name(X509 *a);
int q_X509_verify_cert(X509_STORE_CTX *ctx);
char *q_X509_NAME_oneline(X509_NAME *a, char *b, int c);
EVP_PKEY *q_X509_PUBKEY_get(X509_PUBKEY *a);
void q_X509_STORE_free(X509_STORE *store);
X509_STORE *q_X509_STORE_new();
int q_X509_STORE_add_cert(X509_STORE *ctx, X509 *x);
void q_X509_STORE_CTX_free(X509_STORE_CTX *storeCtx);
int q_X509_STORE_CTX_init(X509_STORE_CTX *ctx, X509_STORE *store,
                          X509 *x509, STACK_OF(X509) *chain);
X509_STORE_CTX *q_X509_STORE_CTX_new();
int q_X509_STORE_CTX_set_purpose(X509_STORE_CTX *ctx, int purpose);
time_t q_getTimeFromASN1(const ASN1_TIME *aTime);

#define q_BIO_get_mem_data(b, pp) (int)q_BIO_ctrl(b,BIO_CTRL_INFO,0,(char *)pp)
#define q_BIO_pending(b) (int)q_BIO_ctrl(b,BIO_CTRL_PENDING,0,NULL)
#ifdef SSLEAY_MACROS // ### verify
#define	q_PEM_read_bio_RSAPrivateKey(bp, x, cb, u) \
        (RSA *)q_PEM_ASN1_read_bio( \
        (char *(*)())d2i_RSAPrivateKey, PEM_STRING_RSA, bp, (char **)x, cb, u)
#define	q_PEM_read_bio_DSAPrivateKey(bp, x, cb, u) \
        (DSA *)q_PEM_ASN1_read_bio( \
        (char *(*)())d2i_DSAPrivateKey, PEM_STRING_DSA, bp, (char **)x, cb, u)
// ### ditto for write
#endif
#define q_SSL_CTX_set_options(ctx,op) q_SSL_CTX_ctrl((ctx),SSL_CTRL_OPTIONS,(op),NULL)
#define q_SKM_sk_num(type, st) ((int (*)(const STACK_OF(type) *))q_sk_num)(st)
#define q_SKM_sk_value(type, st,i) ((type * (*)(const STACK_OF(type) *, int))q_sk_value)(st, i)
#define q_sk_X509_num(st) q_SKM_sk_num(X509, (st))
#define q_sk_X509_value(st, i) q_SKM_sk_value(X509, (st), (i))
#define q_sk_SSL_CIPHER_num(st) q_SKM_sk_num(SSL_CIPHER, (st))
#define q_sk_SSL_CIPHER_value(st, i) q_SKM_sk_value(SSL_CIPHER, (st), (i))
#define q_SSL_CTX_add_extra_chain_cert(ctx,x509) \
        q_SSL_CTX_ctrl(ctx,SSL_CTRL_EXTRA_CHAIN_CERT,0,(char *)x509)
#define q_X509_get_notAfter(x) X509_get_notAfter(x)
#define q_X509_get_notBefore(x) X509_get_notBefore(x)
#define q_EVP_PKEY_assign_RSA(pkey,rsa) q_EVP_PKEY_assign((pkey),EVP_PKEY_RSA,\
					(char *)(rsa))
#define q_EVP_PKEY_assign_DSA(pkey,dsa) q_EVP_PKEY_assign((pkey),EVP_PKEY_DSA,\
					(char *)(dsa))

#endif
