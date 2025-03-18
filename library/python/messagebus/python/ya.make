PY2MODULE(pymessagebus PREFIX lib)

NO_WSHADOW()

OWNER(
    ermolovd
    g:messagebus
)

INCLUDE(${ARCADIA_ROOT}/library/python/messagebus/CMakeLists.inc)

END()
