# coding: utf-8

from libcpp cimport bool as bool_t
from libcpp.pair cimport pair
from libc.stdint cimport uint8_t, uint16_t, int32_t, uint32_t, uint64_t
from libc.time cimport time_t

from enum import IntEnum

from util.datetime.base cimport TInstant
from util.generic.string cimport TString, TStringBuf
from util.generic.vector cimport TVector


cdef extern from "library/cpp/auth_client_parser/oauth_token.h" namespace "NAuthClientParser" nogil:
    ctypedef uint64_t TUid

    cdef cppclass TOAuthToken:
        bool_t Parse(TStringBuf token)
        TUid Uid() const


cdef class OAuthToken:
    cdef TOAuthToken token
    cdef bool_t ok

    def __cinit__(self, strtoken):
        self.ok = self.token.Parse(<TString>strtoken.encode('utf-8'))

    @property
    def ok(self):
        return self.ok

    @property
    def uid(self):
        return self.token.Uid()


cdef extern from "library/cpp/containers/stack_vector/stack_vec.h" nogil:
    cdef cppclass TSmallVec[T](TVector):
        pass


cdef extern from "library/cpp/auth_client_parser/cookie.h" namespace "NAuthClientParser":
    cdef cppclass EParseStatus "NAuthClientParser::EParseStatus":
        pass


cdef extern from "library/cpp/auth_client_parser/cookie.h" namespace "NAuthClientParser::EParseStatus":
    cpdef EParseStatus cInvalid "NAuthClientParser::EParseStatus::Invalid"
    cpdef EParseStatus cNoauthValid "NAuthClientParser::EParseStatus::NoauthValid"
    cpdef EParseStatus cRegularMayBeValid "NAuthClientParser::EParseStatus::RegularMayBeValid"
    cpdef EParseStatus cRegularExpired "NAuthClientParser::EParseStatus::RegularExpired"


class ParseStatus(IntEnum):
    Invalid = <int>cInvalid
    NoauthValid = <int>cNoauthValid
    RegularMayBeValid = <int>cRegularMayBeValid
    RegularExpired = <int>cRegularExpired


cdef extern from "library/cpp/auth_client_parser/cookie.h" namespace "NAuthClientParser" nogil:
    ctypedef uint64_t TTs
    ctypedef uint8_t TTtl
    ctypedef uint8_t TVersion
    ctypedef uint32_t TSocialid
    ctypedef int32_t TPwdCheckDelta
    ctypedef uint16_t TLang
    ctypedef TSmallVec[pair[uint32_t, TStringBuf]] TKeyValue

    cdef cppclass TSessionInfoExt:
        TTtl Ttl
        TTs Ts
        TVersion Version
        TStringBuf Authid
        bool_t IsSafe() const
        bool_t IsSuspicious() const
        bool_t IsStress() const
        TKeyValue Kv

    cdef cppclass TUserInfoExt:
        TUid Uid
        TPwdCheckDelta PwdCheckDelta
        TLang Lang
        TSocialid SocialId
        bool_t IsLite() const
        bool_t HavePwd() const
        bool_t IsStaff() const
        bool_t IsBetatester() const
        bool_t IsGlogouted() const
        bool_t IsPartnerPddToken() const
        bool_t IsSecure() const
        TKeyValue Kv

    cdef cppclass TFullCookie:
        EParseStatus Parse(TStringBuf cookie)
        EParseStatus Parse(TStringBuf cookie, TInstant now)
        EParseStatus Status() const
        TSessionInfoExt SessionInfo() except +
        TUserInfoExt DefaultUser() except +
        TSmallVec[TUserInfoExt] Users() except +


cdef class SessionInfo:
    cdef TSessionInfoExt info

    @property
    def ttl(self):
        return self.info.Ttl

    @property
    def ts(self):
        return self.info.Ts

    @property
    def version(self):
        return self.info.Version

    @property
    def auth_id(self):
        return self.info.Authid.decode('utf-8')

    @property
    def safe(self):
        return self.info.IsSafe()

    @property
    def suspicious(self):
        return self.info.IsSuspicious()

    @property
    def stress(self):
        return self.info.IsStress()

    @property
    def ext_attrs(self):
        return dict([(x.first, x.second.decode('utf-8')) for x in self.info.Kv])


cdef class UserInfo:
    cdef TUserInfoExt info

    @staticmethod
    cdef create(TUserInfoExt info):
        user_info = UserInfo()
        user_info.info = info
        return user_info

    @property
    def uid(self):
        return self.info.Uid

    @property
    def pwd_check_delta(self):
        return self.info.PwdCheckDelta

    @property
    def lang(self):
        return self.info.Lang

    @property
    def social_id(self):
        return self.info.SocialId

    @property
    def lite(self):
        return self.info.IsLite()

    @property
    def have_pwd(self):
        return self.info.HavePwd()

    @property
    def staff(self):
        return self.info.IsStaff()

    @property
    def betatester(self):
        return self.info.IsBetatester()

    @property
    def glogouted(self):
        return self.info.IsGlogouted()

    @property
    def partner_pdd_token(self):
        return self.info.IsPartnerPddToken()

    @property
    def secure(self):
        return self.info.IsSecure()

    @property
    def ext_attrs(self):
        return dict([(x.first, x.second.decode('utf-8')) for x in self.info.Kv])




cdef class Cookie:
    cdef TFullCookie cookie
    cdef TString encoded

    def __cinit__(self, cookie, now=None):
        self.encoded = <TString>cookie.encode('utf-8')
        if now is None:
            self.cookie.Parse(self.encoded)
        else:
            self.cookie.Parse(self.encoded, TInstant.Seconds(now))

    @property
    def status(self):
        return ParseStatus(<int>self.cookie.Status())

    def session_info(self):
        result = SessionInfo()
        result.info = self.cookie.SessionInfo()
        return result

    def default_user(self):
        return UserInfo.create(self.cookie.DefaultUser())

    def users(self):
        result = []
        for userinfo in self.cookie.Users():
            user = UserInfo()
            user.info = userinfo
            result.append(user)
        return result
