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

#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>

// Useful defines
#define SSL_ERRORSTR() QString::fromLocal8Bit(q_ERR_error_string(q_ERR_get_error(), NULL))

/*! \internal

    From OpenSSL's thread(3) manual page:

    OpenSSL can safely be used in multi-threaded applications provided that at
    least two callback functions are set.

    locking_function(int mode, int n, const char *file, int line) is needed to
    perform locking on shared data structures.  (Note that OpenSSL uses a
    number of global data structures that will be implicitly shared
    when-whenever ever multiple threads use OpenSSL.)  Multi-threaded
    applications will crash at random if it is not set.  ...
    ...
    id_function(void) is a function that returns a thread ID. It is not
    needed on Windows nor on platforms where getpid() returns a different
    ID for each thread (most notably Linux)
*/
class OpenSslLocks
{
public:
    inline OpenSslLocks()
    {
        QMutexLocker locker(&locksLocker);
        int numLocks = q_CRYPTO_num_locks();
        locks = new QMutex *[numLocks];
        memset(locks, 0, numLocks * sizeof(QMutex *));
    }
    inline ~OpenSslLocks()
    {
        QMutexLocker locker(&locksLocker);
        for (int i = 0; i < q_CRYPTO_num_locks(); ++i)
            delete locks[i];
        delete [] locks;
    }
    inline QMutex *lock(int num)
    {
        QMutexLocker locker(&locksLocker);
        QMutex *tmp = locks[num];
        if (!tmp)
            tmp = locks[num] = new QMutex;
        return tmp;
    }

    QMutex *globalLock()
    {
        return &locksLocker;
    }

private:
    QMutex locksLocker;
    QMutex **locks;
};
Q_GLOBAL_STATIC(OpenSslLocks, openssl_locks)

extern "C" {
static void locking_function(int mode, int lockNumber, const char *, int)
{
    QMutex *mutex = openssl_locks()->lock(lockNumber);

    // Lock or unlock it
    if (mode & CRYPTO_LOCK)
        mutex->lock();
    else
        mutex->unlock();
}
static unsigned long id_function()
{
    return (unsigned long)QThread::currentThreadId();
}
} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : initialized(false),
      ssl(0),
      ctx(0),
      readBio(0),
      writeBio(0),
      session(0)
{
    if (!q_resolveOpenSslSymbols()) {
        qWarning("QSslSocketBackendPrivate::QSslSocketBackendPrivate: unable to resolve all symbols");
        return;
    }

    // Check if the library itself needs to be initialized.
    openssl_locks()->globalLock()->lock();
    static int q_initialized = false;
    bool doInit = !q_initialized;
    if (doInit)
        q_initialized = true;
    openssl_locks()->globalLock()->unlock();

    if (doInit) {
        // Initialize OpenSSL.
        q_CRYPTO_set_id_callback(id_function);
        q_CRYPTO_set_locking_callback(locking_function);
        q_SSL_library_init();
        q_SSL_load_error_strings();

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            struct {
                int msec;
                int sec;
                void *stack;
            } randomish;

            // This is probably not secure enough.
            randomish.stack = (void *)&randomish;
            randomish.msec = QTime::currentTime().msec();
            randomish.sec = QTime::currentTime().second();
            q_RAND_seed((const char *)&randomish, sizeof(randomish));
        }

        resetGlobalCiphers();
        setGlobalCaCertificates(systemCaCertificates());
    }
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    QStringList descriptionList = descriptionOneLine.split(QLatin1String(" "), QString::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0);
        ciph.d->protocolString = descriptionList.at(1);
        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3);
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3);
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4);
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = cipher->strength_bits;
        ciph.d->supportedBits = cipher->alg_bits;

    }
    return ciph;
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetGlobalCiphers()
{
    bool deleteSsl = false;
    SSL_CTX *myCtx = q_SSL_CTX_new(q_SSLv23_client_method());
    SSL *mySsl = q_SSL_new(myCtx);

    QList<QSslCipher> ciphers;

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(mySsl);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            if (cipher->valid) {
                QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
                if (!ciph.isNull()) {
                    if (!ciph.name().toLower().startsWith(QLatin1String("ADH")))
                        ciphers << ciph;
                }
            }
        }
    }
    
    if (deleteSsl) {
        q_SSL_CTX_free(myCtx);
        q_SSL_free(mySsl);
    }

    setGlobalSupportedCiphers(ciphers);
    setGlobalCiphers(ciphers);
}

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    return certificatesFromPath(QLatin1String("/etc/ssl/certs"));
}

QList<QSslCertificate> QSslSocketPrivate::certificatesFromPath(const QString &path)
{
    // ### FIX: Should support wildcards.
    QFileInfo info(path);
    if (info.isFile()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly))
            return QSslSocketBackendPrivate::QByteArray_to_QSslCertificates(file.readAll());
        return QList<QSslCertificate>();
    }

    QList<QSslCertificate> certs;
    foreach (QString entry, QDir(path).entryList(QDir::Files)) {
        QFile file(path + QLatin1String("/") + entry);
        if (file.open(QIODevice::ReadOnly))
            certs += QSslSocketBackendPrivate::QByteArray_to_QSslCertificates(file.readAll());
    }

    return certs;
}

void QSslSocketBackendPrivate::startClientHandShake()
{
    if (!initOpenSsl()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    testConnection();
    transmit();
}

void QSslSocketBackendPrivate::startServerHandShake()
{
    if (!initOpenSsl()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    testConnection();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    bool transmitting;
    do {
        transmitting = false;
        
        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    // ### Better error handling.
                    q->setErrorString(QSslSocket::tr("Unable to write data: %1").arg(SSL_ERRORSTR()));
                    q->setSocketError(QAbstractSocket::UnknownSocketError);
                    emit q->error(QAbstractSocket::UnknownSocketError);
                    return;
                }
                writeBuffer.free(writtenBytes);
            }
        }

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while ((pendingBytes = q_BIO_pending(writeBio)) > 0) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            plainSocket->write(data.constData(), encryptedBytesRead);
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
            // Read encrypted data from the socket into a buffer.
            data.resize(pendingBytes);
            int decryptedBytesRead = plainSocket->read(data.data(), pendingBytes);

            // Write encrypted data from the buffer into the read BIO.
            q_BIO_write(readBio, data.constData(), decryptedBytesRead);
            transmitting = true;
        }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
            if (testConnection()) {
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
                break;
            }
        }
    
        int readBytes = 0;
        data.resize(4096);
        ::memset(data.data(), 0, data.size());
        do {
            // Don't use SSL_pending(). It's very unreliable.
            if ((readBytes = q_SSL_read(ssl, data.data(), data.size())) > 0) {
                char *ptr = readBuffer.reserve(readBytes);
                ::memcpy(ptr, data.data(), readBytes);
                emit q->readyRead();
                transmitting = true;
                continue;
            }

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
                plainSocket->disconnectFromHost();
                break;
            default:
                // ### Handle errors better.
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(SSL_ERRORSTR()));
                q->setSocketError(QAbstractSocket::UnknownSocketError);
                emit q->error(QAbstractSocket::UnknownSocketError);
                break;
            }
        } while (readBytes > 0);
    } while (transmitting);
}

bool QSslSocketBackendPrivate::testConnection()
{
    Q_Q(QSslSocket);
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            // ### Handle errors better
            q->setErrorString(QSslSocket::tr("Error during SSL handshake: %1").arg(SSL_ERRORSTR()));
            q->setSocketError(QAbstractSocket::UnknownSocketError);
            emit q->error(QAbstractSocket::UnknownSocketError);
        }
        return false;
    }

    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
    peerCertificate = X509_to_QSslCertificate(q_SSL_get_peer_certificate(ssl));

    // This is now.
    QDateTime now = QDateTime::currentDateTime();
    QList<QSslError> errors;

    // Check all certificates in the certificate chain.
    foreach (QSslCertificate cert, peerCertificateChain) {
        // Check for certificate validity
        if (cert.notValidBefore() >= now) {
            errors << QSslError(QSslError::CertificateNotYetValid);
        } else if (cert.notValidAfter() <= now) {
            errors << QSslError(QSslError::CertificateExpired);
        }
    }

    // Check the peer certificate itself.
    if (!peerCertificate.isNull()) {
        QString commonName = peerCertificate.subjectInfo(QSslCertificate::CommonName);
        // ### Both CommonName and AlternameSubjectNames can contain wildcards.
        QString peerName = q->peerName();
        if (commonName != peerName && !peerCertificate.alternateSubjectNames().contains(peerName)) {
            errors << QSslError(QSslError::HostNameMismatch);
        }
    } else {
        errors << QSslError(QSslError::NoPeerCertificate);
    }

    int err;
    switch ((err = q_SSL_get_verify_result(ssl))) {
    case X509_V_OK:
        // X509_V_OK is also reported if the peer had no certificate.
        break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        errors << QSslError(QSslError::UnableToGetIssuerCert); break;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        errors << QSslError(QSslError::UnableToDecryptCertSignature); break;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        errors << QSslError(QSslError::UnableToDecodeIssuerPublicKey); break;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        errors << QSslError(QSslError::CertificateSignatureFailed); break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        errors << QSslError(QSslError::CertificateNotYetValid); break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        errors << QSslError(QSslError::CertificateExpired); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        errors << QSslError(QSslError::InvalidNotBeforeField); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        errors << QSslError(QSslError::InvalidNotAfterField); break;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        errors << QSslError(QSslError::SelfSignedCertificate); break;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        errors << QSslError(QSslError::SelfSignedCertificateInChain); break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        errors << QSslError(QSslError::UnableToGetLocalIssuerCertificate); break;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        errors << QSslError(QSslError::UnableToVerifyFirstCertificate); break;
    case X509_V_ERR_CERT_REVOKED:
        errors << QSslError(QSslError::CertificateRevoked); break;
    case X509_V_ERR_INVALID_CA:
        errors << QSslError(QSslError::InvalidCaCertificate); break;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        errors << QSslError(QSslError::PathLengthExceeded); break;
    case X509_V_ERR_INVALID_PURPOSE:
        errors << QSslError(QSslError::InvalidPurpose); break;
    case X509_V_ERR_CERT_UNTRUSTED:
        errors << QSslError(QSslError::CertificateUntrusted); break;
    case X509_V_ERR_CERT_REJECTED:
        errors << QSslError(QSslError::CertificateRejected); break;
    default:
        errors << QSslError(QSslError::UnspecifiedError); break;
    }

    if (!errors.isEmpty()) {
        ignoreSslErrors = false;
        emit q->sslErrors(errors);
        if (!ignoreSslErrors) {
            plainSocket->disconnectFromHost();
            return false;
        }
    }
    
    connectionEncrypted = true;
    emit q->encrypted();
    return true;
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl)
        q_SSL_shutdown(ssl);
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (ssl) {
        q_SSL_free(ssl);
        ssl = 0;
    }
    if (ctx) {
        // ### Reuse CTX?
        q_SSL_CTX_free(ctx);
        ctx = 0;
    }
}

QSslCipher QSslSocketBackendPrivate::currentCipher() const
{
    if (!ssl || !ctx)
        return QSslCipher();
    SSL_CIPHER *currentCipher = q_SSL_get_current_cipher(ssl);
    return currentCipher ? QSslCipher_from_SSL_CIPHER(currentCipher) : QSslCipher();
}

bool QSslSocketBackendPrivate::resolveSsl()
{
    return q_resolveOpenSslSymbols();
}

bool QSslSocketBackendPrivate::initOpenSsl()
{
    Q_Q(QSslSocket);

    // Create and initialize SSL context. Accept SSLv2, SSLv3 and TLSv1.
    bool client = (mode == QSslSocket::SslClientMode);
    switch (protocol) {
    case QSslSocket::SslV2:
        ctx = q_SSL_CTX_new(client ? q_SSLv2_client_method() : q_SSLv2_server_method());
        break;
    case QSslSocket::SslV3:
        ctx = q_SSL_CTX_new(client ? q_SSLv3_client_method() : q_SSLv3_server_method());
        break;
    case QSslSocket::Compat:
        ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
        break;
    case QSslSocket::TlsV1:
        ctx = q_SSL_CTX_new(client ? q_TLSv1_client_method() : q_TLSv1_server_method());
        break;
    }
    if (!ctx) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL context (%1)").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Enable all bug workarounds.
    q_SSL_CTX_set_options(ctx, SSL_OP_ALL);

    // Initialize ciphers
    QByteArray cipherString;
    int first = true;
    foreach (const QSslCipher &cipher, ciphers.isEmpty() ? globalCiphers() : ciphers) {
        if (first)
            first = false;
        else
            cipherString.append(":");
        cipherString.append(cipher.name().toLatin1());
    }

    if (!q_SSL_CTX_set_cipher_list(ctx, cipherString.data())) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Invalid or empty cipher list (%1)").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Set verify paths
    q_SSL_CTX_set_default_verify_paths(ctx);

    // Load CA certificates.
    foreach (const QSslCertificate &certificate, !localCaCertificates.isEmpty() ? localCaCertificates : globalCaCertificates())
        q_SSL_CTX_use_certificate(ctx, q_X509_dup((X509 *)certificate.handle()));

    // Load private key
    /*
      if (!SSL_CTX_use_PrivateKey_file(d->ctx, QFile::encodeName(d->key).constData(), SSL_FILETYPE_PEM)) {
      setErrorString(tr("Error loading private key, %1").arg(SSL_ERRORSTR()));
      emit error(UnknownSocketError);
            return false;
            }
    */
    
    // Check if the certificate matches the private key.
    /*
      if  (!SSL_CTX_check_private_key(d->ctx)) {
      setErrorString(tr("Private key do not certificate public key, %1").arg(SSL_ERRORSTR()));
      emit error(UnknownSocketError);
      return false;
      }
    */

    // Create and initialize SSL session
    if (!(ssl = q_SSL_new(ctx))) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session, %1").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Clear the session.
    q_SSL_clear(ssl);

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session: %1").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    initialized = true;
    return true;
}

static const char BeginCertString[] = "-----BEGIN CERTIFICATE-----\n";
static const char EndCertString[] = "-----END CERTIFICATE-----\n";

QByteArray QSslSocketBackendPrivate::X509_to_QByteArray(X509 *x509, bool pemEncoded)
{
    if (!x509) {
        qWarning("QSslSocketBackendPrivate::X509_to_QByteArray: null X509");
        return QByteArray();
    }

    // Use i2d_X509 to convert the X509 to an array.
    int length = q_i2d_X509(x509, 0);
    QByteArray array;
    array.resize(length);
    char *data = array.data();
    q_i2d_X509(x509, (unsigned char **)&data);

    if (!pemEncoded)
        return array;

    // Convert to Base64 - wrap at 64 characters.
    array = array.toBase64();
    QByteArray tmp;
    for (int i = 0; i < array.size() - 64; i += 64) {
        tmp += QByteArray::fromRawData(array.data() + i, 64);
        tmp += "\n";
    }
    if (int remainder = array.size() % 64) {
        tmp += QByteArray::fromRawData(array.data() + array.size() - remainder, remainder);
        tmp += "\n";
    }

    return BeginCertString + tmp + EndCertString;
}

static QMap<QString, QString> mapFromOnelineName(char *name)
{
    QMap<QString, QString> info;
    QString issuerInfoStr = QString::fromLocal8Bit(name);
    q_CRYPTO_free(name);

    foreach (QString entry, issuerInfoStr.split(QLatin1String("/"), QString::SkipEmptyParts)) {
        // ### The right-hand encoding seems to allow hex (Regulierungsbeh\xC8orde)
        //entry.replace(QLatin1String("\\x"), QLatin1String("%"));
        //entry = QUrl::fromPercentEncoding(entry.toLatin1());
        
        int splitPos = entry.indexOf(QLatin1String("="));
        if (splitPos != -1) {
            info.insert(entry.left(splitPos), entry.mid(splitPos + 1));
        } else {
            info.insert(entry, QString());
        }
    }

    return info;
}

QSslCertificate QSslSocketBackendPrivate::X509_to_QSslCertificate(X509 *x509)
{
    QSslCertificate certificate;
    if (!x509)
        return certificate;

    // ### Don't use X509_NAME_oneline, at least try QRegexp splitting
    certificate.d->issuerInfo = mapFromOnelineName(q_X509_NAME_oneline(q_X509_get_issuer_name(x509), 0, 0));
    certificate.d->subjectInfo = mapFromOnelineName(q_X509_NAME_oneline(q_X509_get_subject_name(x509), 0, 0));

    ASN1_TIME *nbef = X509_get_notBefore(x509);
    ASN1_TIME *naft = X509_get_notAfter(x509);
    certificate.d->notValidBefore.setTime_t(q_getTimeFromASN1(nbef));
    certificate.d->notValidAfter.setTime_t(q_getTimeFromASN1(naft));
    certificate.d->null = false;
    certificate.d->x509 = q_X509_dup(x509);

    return certificate;
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << X509_to_QSslCertificate(entry);
    }
    return certificates;
}

QSslCertificate QSslSocketBackendPrivate::QByteArray_to_QSslCertificate(const QByteArray &array)
{
    int startPos = array.indexOf(BeginCertString);
    if (startPos == -1)
        return QSslCertificate();
    startPos += sizeof(BeginCertString) - 1;

    int endPos = array.lastIndexOf(EndCertString);
    if (endPos == -1)
        return QSslCertificate();

    QByteArray decoded = QByteArray::fromBase64(QByteArray::fromRawData(array.data() + startPos, endPos - startPos));
    unsigned char *data = (unsigned char *)decoded.data();

    X509 *x509 = q_d2i_X509(0, &data, decoded.size());
    QSslCertificate certificate = X509_to_QSslCertificate(x509);
    q_X509_free(x509);

    return certificate;
}

QList<QSslCertificate> QSslSocketBackendPrivate::QByteArray_to_QSslCertificates(const QByteArray &array)
{
    int offset = 0;
    QList<QSslCertificate> certificates;

    forever {
        int startPos = array.indexOf(BeginCertString, offset);
        if (startPos == -1)
            break;
        startPos += sizeof(BeginCertString) - 1;
        
        int endPos = array.indexOf(EndCertString, startPos);
        if (endPos == -1)
            break;
        offset = endPos + sizeof(EndCertString) - 1;

        QByteArray decoded = QByteArray::fromBase64(QByteArray::fromRawData(array.data() + startPos, endPos - startPos));
        unsigned char *data = (unsigned char *)decoded.data();

        if (X509 *x509 = q_d2i_X509(0, &data, decoded.size())) {
            certificates << X509_to_QSslCertificate(x509);
            q_X509_free(x509);
        }
    }

    return certificates;
}