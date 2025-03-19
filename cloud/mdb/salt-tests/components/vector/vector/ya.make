PACKAGE()

OWNER(g:mdb)

# Execute following commands and update FROM_SANDBOX macro with correct IDs:
#
# export VECTOR_VERSION=<correct packer version>
# curl "https://packages.timber.io/vector/${VECTOR_VERSION}/vector-${VECTOR_VERSION}-x86_64-unknown-linux-gnu.tar.gz" -o vector.tar.gz
# tar -xf vector.tar.gz
# ya upload --ttl=inf --tar ./vector-x86_64-unknown-linux-gnu/bin/vector
# rm -rf vector.tar.gz vector-x86_64-unknown-linux-gnu
# curl "https://packages.timber.io/vector/${VECTOR_VERSION}/vector-${VECTOR_VERSION}-x86_64-apple-darwin.tar.gz" -o vector.tar.gz
# tar -xf vector.tar.gz
# ya upload --ttl=inf --tar ./vector-x86_64-apple-darwin/bin/vector
# rm -rf vector.tar.gz vector-x86_64-apple-darwin

IF(OS_LINUX)
    FROM_SANDBOX(3038686170 OUT_NOAUTO vector EXECUTABLE)
ELSEIF(OS_DARWIN)
    FROM_SANDBOX(3038700606 OUT_NOAUTO vector EXECUTABLE)
ENDIF()

END()
