JAVA_LIBRARY()

JDK_VERSION(11)

OWNER(conterouz)

JAVA_SRCS(SRCDIR src **/*)

SET(jackson_version 2.8.5)

PEERDIR(
    library/java/hnsw/jni
    iceberg/misc
    contrib/java/com/fasterxml/jackson/core/jackson-databind/${jackson_version}
)

LINT(base)
END()

RECURSE(
    ut
)


