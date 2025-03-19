PACKAGE()

OWNER(g:mdb)

# Binaries: https://github.com/uber/prototool/releases
# To upload binaries: ya upload --ttl=inf --tar prototool

IF(OS_LINUX)
    FROM_SANDBOX(2270237239 OUT_NOAUTO prototool EXECUTABLE)
ELSEIF(OS_DARWIN)
    FROM_SANDBOX(2270246186 OUT_NOAUTO prototool EXECUTABLE)
ENDIF()

END()
