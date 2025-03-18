from util.generic.string cimport TString, TStringBuf
from util.system.types cimport ui64
from libcpp cimport bool as TBool
from cython.operator import dereference


cdef TString _to_TString(s):
    s = s.encode("utf-8")
    return TString(<const char*>s, len(s))

cdef TStringBuf _to_TStringBuf(s):
    s = s.encode("utf-8")
    return TStringBuf(<const char*>s, len(s))

cdef extern from "antirobot/lib/spravka.h" namespace "NAntiRobot::TSpravka":
    cdef struct TDegradation:
        TBool Web;
        TBool Market;
        TBool Uslugi;
        TBool Autoru;

cdef extern from "antirobot/lib/spravka.h" namespace "NAntiRobot":
    cdef cppclass TSpravka:
        TBool Parse(const TStringBuf& spravkaBuf, const TStringBuf& domain);
        ui64 Uid;
        TDegradation Degradation;

cdef extern from "antirobot/lib/keyring.h" namespace "NAntiRobot":
    cdef cppclass TKeyRing:
        TKeyRing(const TString& keys);
        @staticmethod
        void SetInstance(TKeyRing instance);

cdef extern from "antirobot/lib/spravka_key.h" namespace "NAntiRobot":
    cdef cppclass TSpravkaKey:
        TSpravkaKey(const TString& keys);
        @staticmethod
        void SetInstance(TSpravkaKey instance);


class Spravka:
    def __init__(self, spravka_str, domain):
        cdef TSpravka spravka;
        self.valid = spravka.Parse(_to_TStringBuf(spravka_str), _to_TStringBuf(domain))
        self.uid = spravka.Uid
        self.expired = False
        self.issueDate = ''
        self.ip = ''
        self.reason = ''
        self.degradation = spravka.Degradation


def init_keyring(keys_content):
    cdef TKeyRing* keyRing
    keyRing = new TKeyRing(_to_TString(keys_content))
    TKeyRing.SetInstance(dereference(keyRing))

def init_spravka_key(key_content):
    cdef TSpravkaKey* key
    key = new TSpravkaKey(_to_TString(key_content))
    TSpravkaKey.SetInstance(dereference(key))
