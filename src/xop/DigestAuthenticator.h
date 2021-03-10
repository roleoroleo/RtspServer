//PHZ
//2019-10-6

#ifndef RTSP_DIGEST_AUTHENTICATION_H
#define RTSP_DIGEST_AUTHENTICATION_H

#include "Authenticator.h"

#include <cstdint>
#include <string>

namespace xop
{

class DigestAuthenticator : public Authenticator
{
public:
	DigestAuthenticator(std::string realm, std::string username, std::string password);
	virtual ~DigestAuthenticator();

	std::string GetRealm() const
	{ return realm_; }

	std::string GetUsername() const
	{ return username_; }

	std::string GetPassword() const
	{ return password_; }

	std::string GetNonce();
	std::string GetResponse(std::string nonce, std::string cmd, std::string url);

  bool Authenticate(RtspRequest *rtsp_request, std::string &nonce);

private:
	std::string realm_;
	std::string username_;
	std::string password_;

};

}

#endif
