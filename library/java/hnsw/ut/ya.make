JTEST()

JDK_VERSION(11)

OWNER(conterouz)

SET(junit_version 4.9)

JAVA_SRCS(SRCDIR src **/*)

PEERDIR(
    library/java/hnsw
    library/java/hnsw/jni
    iceberg/commune-offheap
    contrib/java/junit/junit/${junit_version}
)

IF (SANITIZER_TYPE)
    TAG(ya:not_autocheck)
ENDIF()

IF(JDK_VERSION != "8")
    JAVAC_FLAGS(
        --add-exports=java.base/sun.nio.ch=ALL-UNNAMED
    )
    JVM_ARGS(
        --add-exports=java.base/sun.nio.ch=ALL-UNNAMED
        --add-exports=java.base/jdk.internal.misc=ALL-UNNAMED
    )
ENDIF()

LINT(base)
END()
