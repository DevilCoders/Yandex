JAVA_LIBRARY()

JDK_VERSION(11)

OWNER(g:passport_infra)

PEERDIR(
    library/java/tvmauth
)

JAVA_SRCS(Create.java)

LINT(base)
END()
