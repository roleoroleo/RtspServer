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

  virtual bool Authenticate(std::shared_ptr<RtspRequest> request, const std::string& nonce) = 0;
  virtual size_t GetFailedResponse(std::shared_ptr<RtspRequest> request, std::shared_ptr<char> buf, size_t size, std::string& nonce) = 0;

private:

};

}

#endif
