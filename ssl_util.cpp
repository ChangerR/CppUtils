#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ssl_util.h"

static bool ssl_lib_initialed = false;

inline int ssl_err_message_cb(const char *str, size_t len, void *u)
{
	std::string* msg = (std::string*)u;
	msg->append(str,len);
	return len;
}

const int ssl_client_flag = 1;
const int ssl_server_flag = 1 << 2;

SSLFactory::SSLFactory() : ctx_(NULL), flag_(0) 
{
	if (!ssl_lib_initialed)
	{
		SSL_library_init();
		SSL_load_error_strings();
	}
}

SSLFactory::~SSLFactory()
{
	if (ctx_)
	{
		SSL_CTX_free(ctx_);
		ctx_ = NULL;
	}
}

bool SSLFactory::load()
{
	 ctx_ = SSL_CTX_new(SSLv23_client_method());
	 if (!ctx_)
	 {
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		return false;
	 }
	 flag_ = ssl_client_flag;
	 return true;
}

bool SSLFactory::load(const std::string& cert,const std::string& priv)
{
	ctx_ = SSL_CTX_new(SSLv23_client_method());
	if (!ctx_)
	{
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		ctx_ = NULL;
		return false;
	}
	if (SSL_CTX_use_certificate_file(ctx_, cert.c_str(), SSL_FILETYPE_PEM) != 1)
	{
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		SSL_CTX_free(ctx_);
		ctx_ = NULL;
		return false;
	}

    if (SSL_CTX_use_PrivateKey_file(ctx_, priv.c_str(), SSL_FILETYPE_PEM) != 1)
	{
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		SSL_CTX_free(ctx_);
		ctx_ = NULL;
		return false;
	}
	
	if (SSL_CTX_check_private_key(ctx_) != 1)
	{
		err_message_ = "cert is not match private key";
		SSL_CTX_free(ctx_);
		ctx_ = NULL;
		return false;
	}
	SSL_CTX_set_options(ctx_, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
	flag_ = ssl_client_flag | ssl_server_flag;
	return true;
}

SSLBase* SSLFactory::create_client()
{
	if (!(flag_ & ssl_client_flag))
		return NULL;
	SSLBase* base = new SSLClient();
	if (!base->init(ctx_))
	{
		err_message_ = base->get_error();
		delete base;
		base = NULL;
	}
	return base;
}

SSLBase* SSLFactory::create_server()
{
	if (!(flag_ & ssl_server_flag))
		return NULL;
	SSLBase* base = new SSLServer();
	if (!base->init(ctx_))
	{
		err_message_ = base->get_error();
		delete base;
		base = NULL;
	}
	return base;
}

SSLBase::~SSLBase()
{
	if (ssl_)
	{
		SSL_shutdown(ssl_);
		SSL_free(ssl_);
	}
}

bool SSLBase::is_handshake_ok()
{
	return SSL_is_init_finished(ssl_) && is_handshake_;
}

int SSLBase::handshake(unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& send)
{	
	int ret;
	
	if (!is_client_ && (buf == NULL || len <=0 ))
		return SSL_WRITE_HANDSHAKE_WANTIO;
	
	if (buf != NULL && len > 0)
		BIO_write(rbio_,buf,len);
	
	if (is_client_)
		ret = SSL_do_handshake(ssl_);
	else
		ret = SSL_accept(ssl_);
	if (ret == 1)
	{
		is_handshake_ = true;
	}
	else
	{
		switch (SSL_get_error(ssl_, ret))
		{
		case SSL_ERROR_NONE:
			is_handshake_ = true;
			break;
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_READ:
			len = SSL_WRITE_HANDSHAKE_WANTIO;
			break;
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_SYSCALL:
		default:
			ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
			return SSL_WRITE_HANDSHAKE_FAIL;
		}
	}

	char buffer[4096];
	int rlen;
	do {
		rlen = BIO_read(wbio_,buffer,sizeof(buffer));
		if (rlen > 0 && send != nullptr)
		{
			send((unsigned char*)buffer,rlen);
		}
	} while(rlen > 0);
	return len;
}

int SSLBase::write(const unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& send)
{
	int sw_len;
	int orign_len = len;
	int encode_len;
	char buffer[4096];
	do {	
		sw_len = SSL_write(ssl_,buf,len);
		if (sw_len < 0)
		{
			ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
			return sw_len;
		}
		buf += sw_len;
		len -= sw_len;
		do {
			encode_len = BIO_read(wbio_,buffer,sizeof(buffer));
			if (encode_len > 0 && send != nullptr)
			{
				send((unsigned char*)buffer,encode_len);
			}
		} while(encode_len > 0);
	} while(len > 0);
	return orign_len;
}

int SSLBase::read(const unsigned char* buf,int len,const std::function<void (const unsigned char*,int)>& rb)
{
	int sw_len;
	int orign_len = len;
	int decode_len;
	char buffer[4096];
	do {	
		sw_len = BIO_write(rbio_,buf,len);
		if (sw_len < 0)
		{
			ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
			return sw_len;
		}
		buf += sw_len;
		len -= sw_len;
		do {
			decode_len = SSL_read(ssl_,buffer,sizeof(buffer));
			if (decode_len > 0 && rb != nullptr)
			{
				rb((unsigned char*)buffer,decode_len);
			}
		} while(decode_len > 0);
	} while(len > 0);
	return orign_len;
}

const std::string& SSLBase::get_error() const
{
	return err_message_;
}

const std::string SSLBase::get_peer_cert()
{
	std::string ret;
	if (ssl_)
	{
		X509* cert = SSL_get_peer_certificate(ssl_);
		if (cert)
		{
			char* buffer;
			long len;
			BIO* pem = BIO_new(BIO_s_mem());
			PEM_write_bio_X509(pem, cert);
			len = BIO_get_mem_data(pem,&buffer);
			ret.append(buffer,len);
			BIO_free(pem);
			X509_free(cert);
		}
	}
	return std::move(ret);
}
		
bool SSLClient::init(SSL_CTX* ctx)
{
	if ( !ctx )
	{
		return false;
	}
	ssl_ = SSL_new(ctx);
	if (!ssl_)
	{
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		return false;
	}
	rbio_ = BIO_new(BIO_s_mem());
	wbio_ = BIO_new(BIO_s_mem());
	
	SSL_set_bio(ssl_,rbio_,wbio_);
	SSL_set_connect_state(ssl_);
	return true;
}

bool SSLServer::init(SSL_CTX* ctx)
{
	if ( !ctx )
	{
		return false;
	}
	ssl_ = SSL_new(ctx);
	if (!ssl_)
	{
		ERR_print_errors_cb(ssl_err_message_cb,&err_message_);
		return false;
	}
	rbio_ = BIO_new(BIO_s_mem());
	wbio_ = BIO_new(BIO_s_mem());
	
	SSL_set_bio(ssl_,rbio_,wbio_);
	SSL_set_accept_state(ssl_);
	return true;
}