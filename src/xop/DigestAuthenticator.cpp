#include "DigestAuthenticator.h"
#include "md5/md5.hpp" 

using namespace xop;

DigestAuthenticator::DigestAuthenticator(std::string realm, std::string username, std::string password)
	: realm_(realm)
	, username_(username)
	, password_(password)
{

}

DigestAuthenticator::~DigestAuthenticator()
{

}

std::string DigestAuthenticator::GetNonce()
{
	return md5::generate_nonce();
}

std::string DigestAuthenticator::GetResponse(std::string nonce, std::string cmd, std::string url)
{
	//md5(md5(<username>:<realm> : <password>) :<nonce> : md5(<cmd>:<url>))

	auto hex1 = md5::md5_hash_hex(username_ + ":" + realm_ + ":" + password_);
	auto hex2 = md5::md5_hash_hex(cmd + ":" + url);
	auto response = md5::md5_hash_hex(hex1 + ":" + nonce + ":" + hex2);
	return response;
}

bool DigestAuthenticator::Authenticate(RtspRequest *rtsp_request, std::string &nonce)
{
  std::string cmd = rtsp_request->MethodToString[rtsp_request->GetMethod()];
  std::string url = rtsp_request->GetRtspUrl();

  if (nonce.size() > 0 && (GetResponse(nonce, cmd, url) == rtsp_request->GetAuthResponse())) {
    return true;
  } else {
#if 0
    std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
    //_nonce = auth_info_->GetNonce();
    int size = rtsp_request->BuildUnauthorizedRes(res.get(), 4096, realm_.c_str(), _nonce.c_str());
    SendRtspMessage(res, size);
#endif
    return false;
  }
}


