/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QTRANSPORTAUTH_QWS_H
#define QTRANSPORTAUTH_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_MODULE(Gui)

// Uncomment to generate debug output
// #define QTRANSPORTAUTH_DEBUG 1

#define KEY_TEMPLATE "XOXOXOauthOXOXOX99"
#define APP_KEY const char *_key = KEY_TEMPLATE;
#define QL_APP_KEY const char *_ql_key = KEY_TEMPLATE;

#define KEY_LEN 16
#define MAGIC_BYTES 4
#define MAX_PROG_ID 255

// Number of bytes of each message to authenticate.  Just need to ensure
// that the command at the beginning hasn't been tampered with.  This value
// does not matter for trusted transports.
#define AMOUNT_TO_AUTHENTICATE 200

/**
  \internal
  \class AuthCookie
  Struct to carry process authentication key and id
*/
#define HEADER_LEN 24

/**
  \macro AUTH_ID
  Macro to manage authentication header.  Format of header is:
  \table
  \header \i BYTES  \i  CONTENT
     \row \i 0-3    \i  magic numbers
     \row \i 4      \i  length of authenticated data (max 255 bytes)
     \row i\ 5      \i  reserved
     \row \i 6-21   \i  MAC digest, or shared secret in case of simple auth
     \row \i 22     \i  program id
     \row \i 23     \i  sequence number
  \endtable
  Total length of the header is 24 bytes

  However this may change.  Instead of coding these numbers use the AUTH_ID,
  AUTH_KEY, AUTH_DATA and AUTH_SPACE macros.
*/

#define AUTH_ID(k) ((unsigned char)(k[KEY_LEN]))
#define AUTH_KEY(k) ((unsigned char *)(k))

// must be a largish -ve number under any endianess when cast as an int
const unsigned char magic[MAGIC_BYTES] = { 0xBA, 0xD4, 0xD4, 0xBA };
const int magicInt = 0xBAD4D4BA;

#define AUTH_DATA(x) (unsigned char *)((x) + HEADER_LEN)
#define AUTH_SPACE(x) ((x) + HEADER_LEN)
#define LEN_IDX 4
#define KEY_IDX 6
#define PROG_IDX 22
#define SEQ_IDX 23

#define KEYFILE "keyfile"

/**
  Header in above format, less the magic bytes.
  Useful for reading off the socket
*/
struct AuthHeader
{
    unsigned char len;
    unsigned char pad;
    unsigned char digest[KEY_LEN];
    unsigned char id;
    unsigned char seq;
};

/**
  Header in a form suitable for authentication routines
*/
struct AuthMessage
{
    AuthMessage()
    {
        ::memset( authData, 0, sizeof(authData) );
        ::memcpy( pad_magic, magic, MAGIC_BYTES );
    }
    unsigned char pad_magic[MAGIC_BYTES];
    union {
        AuthHeader hdr;
        char authData[sizeof(AuthHeader)];
    };
    char payLoad[AMOUNT_TO_AUTHENTICATE];
};

/**
  Auth data as written to the key file
*/
struct AuthCookie
{
    unsigned char key[KEY_LEN];
    unsigned char pad;
    unsigned char progId;
};


/**
  \class QTransportAuth
  \brief Authenticate a message transport.
  For performance reasons, message authentication is tied to an individual
  message transport instance.  For example in connection oriented transports
  the authentication cookie can be cached against the connection avoiding
  the overhead of authentication on every message.

  For each process there is one instance of the QTransportAuth object.
  For server processes it can determine the \link secure-exe-environ.html SXV
  Program Identity \endlink and provide access to policy data to determine if
  the message should be forwarded for action.  If not actioned, the message
  may be treated as being from a flawed or malicious process.

  Retrieve the instance with the getInstance() method.  The constructor is
  disabled and instances of QTransportAuth should never be constructed by
  calling classes.

  Check the authorization using the checkAuth() method.

  For client processes authentication information can be added using the
  addAuth() method.  Note that if the transport is connection oriented
  it will be assumed that the server has cached the authentication infor-
  mation and this method will be a no-op.

  Generally these methods should never need to be called except by lower
  level message handling logic.
*/
class Q_GUI_EXPORT QTransportAuth : public QObject
{
    Q_OBJECT
public:
    static QTransportAuth *getInstance();

    static const char *errorStrings[];

    enum Result {
        // Error codes
        Pending = 0x00,
        TooSmall = 0x01,
        CacheMiss = 0x02,
        NoMagic = 0x03,
        NoSuchKey = 0x04,
        FailMatch = 0x05,
        // reserved for expansion
        Success = 0x1e,
        ErrMask = 0x1f,

        // Verification codes
        Allow = 0x20,
        Deny = 0x40,
        Ask = 0x60,
        // reserved
        StatusMask = 0xe0
    };

    enum Properties {
        Trusted = 0x01,
        Connection = 0x02,
        UnixStreamSock = 0x04,
        SharedMemory = 0x08,
        MessageQueue = 0x10,
        UDP = 0x20,
        TCP = 0x40,
        UserDefined = 0x80,
        TransportType = 0xfc
    };

    struct Data
    {
        unsigned char properties;
        unsigned char progId;
        unsigned char status;
        unsigned int descriptor;   // socket fd or shmget key

        bool operator==( const Data &rhs ) const
        {
            return ((( this->properties & TransportType ) ==
                        ( rhs.properties & TransportType )) &&
                    this->descriptor == rhs.descriptor );
        }
        const char *error() const
        {
            if  (( status & ErrMask ) <= FailMatch )
                return ( errorStrings[int( status & ErrMask )] );
            else
                return "Success";
        }
    };

    void connectTransport( unsigned char properties, int descriptor );
    bool addAuth( char *msg, int msgLen, Data d );
    bool checkAuth( char *msg, int msgLen, Data &d );
    void setKeyFilePath( const QString & );
    QString keyFilePath() const { return m_keyFilePath; }
    void setProcessKey( const char * );

    Result authFromSocket( unsigned char properties, int descriptor, char *msg, int len );
    void authToSocket( unsigned char properties, int descriptor, char *msg, int len );

    bool trusted( QTransportAuth::Data );
    void setTrusted( bool, QTransportAuth::Data & );
    bool connection( QTransportAuth::Data );
    void setConnection( bool, QTransportAuth::Data & );

Q_SIGNALS:
    void authViolation( QTransportAuth::Data );
    void policyCheck( QTransportAuth::Data );
public Q_SLOTS:
    void closeTransport(  unsigned char properties, int descriptor );
private:
    // users should never construct their own
    QTransportAuth();
    ~QTransportAuth();
    void freeCache();
    const unsigned char *getClientKey( unsigned char progId );

    bool keyInitialised;
    QString m_keyFilePath;
    AuthCookie authKey;
    QList<Data> data;
};

uint qHash( QTransportAuth::Data );

int hmac_md5(
        unsigned char*  text,         /* pointer to data stream */
        int             text_length,  /* length of data stream */
        const unsigned char*  key,    /* pointer to authentication key */
        int             key_length,   /* length of authentication key */
        unsigned char * digest        /* caller digest to be filled in */
        );


#ifdef QTRANSPORTAUTH_DEBUG
void stringify_key( char *buf, const unsigned char* key, size_t sz );
#endif

#endif // QTRANSPORTAUTH_QWS_H
