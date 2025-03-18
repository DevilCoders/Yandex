JTEST()

PEERDIR(
    library/java/ds/pgdriver

    contrib/java/org/junit/jupiter/junit-jupiter-engine/5.8.2
    contrib/java/org/junit/jupiter/junit-jupiter-params/5.8.2
)

JAVA_SRCS(SRCDIR java **/*)

LINT(base)
END()
