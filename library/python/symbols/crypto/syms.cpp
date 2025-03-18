#include <library/python/symbols/registry/syms.h>

#include <contrib/libs/openssl/include/openssl/bn.h>

BEGIN_SYMS("crypto")
SYM(BN_CTX_free)
SYM(BN_CTX_new)
SYM(BN_bin2bn)
SYM(BN_bn2bin)
SYM(BN_free)
SYM(BN_mod_inverse)
SYM(BN_new)
SYM(BN_num_bits)
SYM(BN_set_negative)
END_SYMS()
