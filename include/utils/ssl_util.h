#ifndef __SSL_CLIENT_H
#define __SSL_CLIENT_H
#include <string>
#include <functional>

const int SSL_WRITE_HANDSHAKE_WANTIO = 0;
const int SSL_WRITE_HANDSHAKE_FAIL = -1;

struct ssl_st;
typedef struct ssl_st SSL;
struct bio_st;
typedef struct bio_st BIO;
struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;
class SSLBase;

class SSLFactory
{
public:
	SSLFactory();
	virtual ~SSLFactory();
	
	bool load();
	
	bool load(const std::string& cert,const std::string& priv);
	
	SSLBase* create_client();
	
	SSLBase* create_server();
	
	const std::string get_error() const
	{
		return err_message_;
	}	
private:
	SSL_CTX* ctx_;
	int flag_;
	std::string err_message_;
};

class SSLBase
{
public:
	SSLBase() : ssl_(NULL),is_handshake_(false),rbio_(NULL),wbio_(NULL) {}
	
	virtual ~SSLBase();
	
	virtual bool is_handshake_ok();
	
	virtual int handshake(unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& send);
	
	virtual int write(const unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& send);
	
	virtual int read(const unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& rb);
	
	virtual const std::string& get_error() const;
	
	virtual const std::string get_peer_cert();
protected:
	virtual bool init(SSL_CTX* ctx) = 0;
protected:	
	SSL* ssl_;
	bool is_handshake_;
	std::string err_message_;
	BIO* rbio_;
	BIO* wbio_;
	bool is_client_;
	friend class SSLFactory;
};

class SSLClient : public SSLBase
{
public:
	virtual ~SSLClient() {}
	
protected:
	SSLClient() { is_client_ = true; }
	bool init(SSL_CTX* ctx);
	friend class SSLFactory;
};

class SSLServer : public SSLBase
{
public:
	virtual ~SSLServer() {}
	
protected:
	SSLServer() { is_client_ = false; }
	bool init(SSL_CTX* ctx);
	friend class SSLFactory;
};
#endif