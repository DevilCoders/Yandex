JTEST()

JDK_VERSION(11)

OWNER(g:cloud-iam)

JAVA_SRCS(
    SRCDIR src/test/java **/*
)

PEERDIR(
    cloud/iam/codegen/java/lib
    contrib/java/junit/junit/4.12
)

LINT(base)
END()
