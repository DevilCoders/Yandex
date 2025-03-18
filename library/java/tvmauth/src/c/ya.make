DLL(tvmauth_java)

EXPORTS_SCRIPT(tvmauth.exports)

OWNER(g:passport_infra)

SRCS(
    ru_yandex_passport_tvmauth_deprecated_ServiceContext.cpp
    ru_yandex_passport_tvmauth_deprecated_UserContext.cpp
    ru_yandex_passport_tvmauth_DynamicClient.cpp
    ru_yandex_passport_tvmauth_internal_LogFetcher.cpp
    ru_yandex_passport_tvmauth_NativeTvmClient.cpp
    ru_yandex_passport_tvmauth_TvmApiSettings.cpp
    ru_yandex_passport_tvmauth_TvmToolSettings.cpp
    ru_yandex_passport_tvmauth_Unittest.cpp
    ru_yandex_passport_tvmauth_Utils.cpp
    ru_yandex_passport_tvmauth_Version.cpp
    util.cpp
)

IF (OS_LINUX)
    LDFLAGS(-Wl,-z,noexecstack)
ENDIF()

PEERDIR(
    contrib/libs/jdk
    library/cpp/tvmauth/client
    library/cpp/tvmauth/client/misc/api/dynamic_dst
)

END()
