JAVA_LIBRARY(ci-common-bazinga)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application
    ci/common/utils

    iceberg/commune-bazinga-ydb

    contrib/java/org/mongodb/bson
    contrib/java/org/slf4j/slf4j-api
    contrib/java/com/amazonaws/aws-java-sdk-s3
    contrib/java/org/lz4/lz4-java
)

END()

RECURSE_FOR_TESTS(
    src/test
)
