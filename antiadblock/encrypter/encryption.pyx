# -*- coding: utf-8 -*-

from util.generic.string cimport TString, TStringBuf
from util.generic.ptr cimport THolder, MakeHolder
from util.generic.maybe cimport TMaybe
from cython.operator cimport dereference as deref


cdef extern from "yweb/webdaemons/icookiedaemon/icookie_lib/icookie.h" namespace "NIcookie":
    cdef cppclass TKeysSet:
        pass


cdef extern from "yweb/webdaemons/icookiedaemon/icookie_lib/keys.h" namespace "NIcookie":
    TKeysSet ReadKeysFromFile(const TString& fileName)


cdef extern from "antiadblock/encrypter/encryption.h" namespace "NAntiAdBlock":
    const TMaybe[TString] encryptCookie(const TStringBuf& value, const TKeysSet& keys)
    const TMaybe[TString] decryptCookie(const TStringBuf& value, const TKeysSet& keys)
    const TMaybe[TString] decryptCrookieWithDefaultKey(const TStringBuf& value)


cdef class CookieEncrypter:
    cdef THolder[TKeysSet] key_set

    def __cinit__(self, bytes keys_file_path):
        self.key_set = MakeHolder[TKeysSet](ReadKeysFromFile(keys_file_path))

    def encrypt_cookie(self, bytes cookie_value):
        cdef TStringBuf cValue = TStringBuf(cookie_value, len(cookie_value))
        cdef TMaybe[TString] resultMaybe = encryptCookie(cValue, deref(self.key_set.Get()))
        if(resultMaybe.Empty()):
            return None
        cdef TString encrypted_value = resultMaybe.GetRef()
        return encrypted_value

    def decrypt_cookie(self, bytes cookie_value):
        cdef TStringBuf cValue = TStringBuf(cookie_value, len(cookie_value))
        cdef TMaybe[TString] resultMaybe = decryptCookie(cValue, deref(self.key_set.Get()))
        if(resultMaybe.Empty()):
            return None
        cdef TString decrypted_value = resultMaybe.GetRef()
        return decrypted_value


def decrypt_crookie_with_default_keys(bytes crookie_value):
    cdef TStringBuf cValue = TStringBuf(crookie_value, len(crookie_value))
    cdef TMaybe[TString] resultMaybe = decryptCrookieWithDefaultKey(cValue)
    if(resultMaybe.Empty()):
        return None
    cdef TString decrypted_value = resultMaybe.GetRef()
    return decrypted_value
