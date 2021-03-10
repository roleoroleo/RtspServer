#ifndef RTSP_AUTHENTICATOR_H
#define RTSP_AUTHENTICATOR_H

#include <string>
#include "RtspMessage.h"

namespace xop
{

class Authenticator
{
public:
	Authenticator() {};
	virtual ~Authenticator() {};

  virtual bool Authenticate(std::shared_ptr<RtspRequest> request, std::string &nonce) = 0;

private:

};

}

#endif
