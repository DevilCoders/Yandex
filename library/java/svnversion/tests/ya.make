JTEST()

JDK_VERSION(11)

OWNER(g:ymake)

PEERDIR(
    library/java/svnversion
    contrib/java/junit/junit/4.12
)

JAVA_SRCS(*.java)

LINT(base)
END()
