from util.generic.maybe cimport TMaybe
from util.generic.string cimport TString
from util.generic.string cimport TStringBuf
from libcpp cimport bool
from libc.stdint cimport uint64_t, uint16_t, uint8_t

from util.system.types cimport i8, ui8, ui16, i64, ui64
from util.datetime.base cimport TInstant


cdef extern from "util/datetime/base.h" nogil:
    cdef cppclass TInstant:
        TString ToStringLocal() const;

    cdef TInstant Now();


cdef extern from "kernel/turbo/login/lib/crypto/key_provider.h" namespace "NTurboLogin":
    cdef cppclass TKeyProvider:
        TKeyProvider(const TString& secret) except +
        ui16 GetCurrentKeyId(TInstant now)


cdef class pyKeyProvider:
    cdef TKeyProvider *thisptr
    cdef ui16 keyId

    def __cinit__(self, TString key):
        self.thisptr = new TKeyProvider(key)
        keyId = self.thisptr.GetCurrentKeyId(Now())





cdef extern from "kernel/turbo/login/lib/crypto/crypto_arc.h":
    bool EncryptYuid(const TKeyProvider* keyProvider, uint16_t keyId, const TStringBuf& yuid, const TStringBuf& domain, TString& encrypted);
    bool DecryptYuid(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TString& yuid, TString& domain);


def encrypt_uid(key, keyid, domain, yuid):
    cdef TString encrypted
    keyProvider = pyKeyProvider(key)
    cdef bool bool_res = EncryptYuid(keyProvider.thisptr, keyid, yuid, domain, encrypted)
    return bool_res, encrypted

def decrypt_uid(key, encrypted):
    cdef TString yuid
    cdef TString domain
    keyProvider = pyKeyProvider(key)
    cdef bool bool_res = DecryptYuid(keyProvider.thisptr, encrypted, yuid, domain)
    return bool_res, yuid, domain