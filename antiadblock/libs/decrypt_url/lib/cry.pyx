# -*- coding: utf-8 -*-
import sys
import traceback

from libcpp cimport bool
from libcpp.pair cimport pair
from util.generic.string cimport TString, TStringBuf


class DecryptionError(StandardError):

    def __init__(self, msg, cause, *args, **kwargs):
        super(DecryptionError, self).__init__(msg, *args, **kwargs)
        self.cause = cause

def raise_decryption_error_with_cause(msg):
    raise DecryptionError(msg, cause=traceback.format_exception(*sys.exc_info(), limit=2))


cdef extern from "antiadblock/libs/decrypt_url/lib/cry.h" namespace "NAntiAdBlock":
    cdef struct TDecryptResult:
            TString url
            TString seed
            TString origin
    const TString GetKey(TStringBuf secret, TStringBuf data)
    const TString DecryptBase64(const TStringBuf data)
    const TString DecryptXor(const TStringBuf data, const TStringBuf key)
    bool IsCryptedUrl(const TStringBuf& crypted_url, const TStringBuf& crypt_prefix)
    const pair[TString, TString] ResplitUsingLength(int length, const TStringBuf& cry_part, const TStringBuf& noncry_part) except +
    const TDecryptResult DecryptUrl(const TStringBuf& crypted_url, const TStringBuf& secret_key,
                                           const TStringBuf& crypt_prefix, int is_trailing_slash_enabled) except +
    const int SEED_LENGTH
    const int URL_LENGTH_PREFIX_LENGTH


SEED_LENGTH_EXPORT = SEED_LENGTH
URL_LENGTH_PREFIX_LENGTH_EXPORT = URL_LENGTH_PREFIX_LENGTH


def get_key(bytes secret, bytes data):
    cdef TStringBuf cSecret = TStringBuf(secret, len(secret))
    cdef TStringBuf cData = TStringBuf(data, len(data))

    cdef TString key = GetKey(cSecret, cData)
    return key

def decrypt_base64(bytes data):
    cdef TStringBuf cData = TStringBuf(data, len(data))

    cdef TString decrypted = DecryptBase64(cData)
    return decrypted

def decrypt_xor(bytes data, bytes key):
    cdef TStringBuf cData = TStringBuf(data, len(data))
    cdef TStringBuf cKey = TStringBuf(key, len(key))

    cdef TString decrypt = DecryptXor(cData, cKey)
    return decrypt

def is_crypted_url(bytes crypted, bytes prefix):
    cdef TStringBuf cCrypted = TStringBuf(crypted, len(crypted))
    cdef TStringBuf cPrefix = TStringBuf(prefix, len(prefix))
    return IsCryptedUrl(cCrypted, cPrefix)

def resplit_using_length(int length, bytes cry_part, bytes noncry_part=b""):
    cdef TStringBuf cCry_part = TStringBuf(cry_part, len(cry_part))
    cdef TStringBuf cNoncry_part = TStringBuf(noncry_part, len(noncry_part))
    cdef pair[TString, TString] resplitted
    try:
        resplitted = ResplitUsingLength(length, cCry_part, cNoncry_part)
        return resplitted.first, resplitted.second
    except Exception as e:
        raise_decryption_error_with_cause("Failed to decrypt url, {}".format(str(e)))

def decrypt_url(bytes crypted, bytes secret, bytes prefix, is_trailing_slash_enabled):
    cdef TStringBuf cCrypted = TStringBuf(crypted, len(crypted))
    cdef TStringBuf cSecret = TStringBuf(secret, len(secret))
    cdef TStringBuf cPrefix = TStringBuf(prefix, len(prefix))

    cdef TDecryptResult decrypted_value
    try:
        decrypted_value = DecryptUrl(cCrypted, cSecret, cPrefix, is_trailing_slash_enabled)
        if decrypted_value.url == "":
            return None, None, None
        seed = decrypted_value.seed or None
        origin = decrypted_value.origin or None
        return decrypted_value.url, seed, origin
    except Exception as e:
        raise_decryption_error_with_cause("Failed to decrypt url, {}".format(str(e)))
