// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dictionary.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <openssl/sha.h>

namespace sdch {

namespace {

#if nginx_version < 1006000
static void
ngx_encode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
	ngx_encode_base64(dst, src);
	unsigned i;

	for (i = 0; i < dst->len; i++) {
		if (dst->data[i] == '+')
			dst->data[i] = '-';
		if (dst->data[i] == '/')
			dst->data[i] = '_';
	}
}
#endif

void encode_id(u_char* sha, Dictionary::id_t& id) {
	ngx_str_t src = {6, sha};
	ngx_str_t dst = {8, id.data()};
	ngx_encode_base64url(&dst, &src);
}

void get_dict_ids(const char* buf,
                  size_t buflen,
                  Dictionary::id_t& client_id,
                  Dictionary::id_t& server_id) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, buf, buflen);
  unsigned char sha[32];
  SHA256_Final(sha, &ctx);

  encode_id(sha, client_id);
  encode_id(sha + 6, server_id);
}


}  // namespace

bool Dictionary::init(const char* begin,
                      const char* payload,
                      const char* end) {
  if (begin == NULL || payload == NULL || end == NULL)
    return false;

  hashed_dict_.reset(new open_vcdiff::HashedDictionary(payload, end - payload));
  if (!hashed_dict_->Init())
    return false;

  get_dict_ids(begin, end - begin, client_id_, server_id_);
  size_ = end - begin;
  return true;
}

}  // namespace sdch
