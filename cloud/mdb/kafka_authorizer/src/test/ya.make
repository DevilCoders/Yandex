JUNIT5()

JDK_VERSION(11)

# FIXME Discrepancy between the available test versions by dependencies with the launcher version was found.
# Remove ENV macro below to reproduce the problem.
# For more info see https://st.yandex-team.ru/DEVTOOLSSUPPORT-7454#6128ec627e6507138f034e45
ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)

JAVA_SRCS(SRCDIR java **/*)

SIZE(SMALL)

PEERDIR(
    cloud/mdb/kafka_authorizer
    contrib/java/junit/junit/4.13.2
    contrib/java/org/mockito/mockito-core/2.18.3
    contrib/java/org/junit/jupiter/junit-jupiter-api/5.5.2
    contrib/java/org/junit/jupiter/junit-jupiter-params/5.5.2
    contrib/java/org/junit/jupiter/junit-jupiter-engine/5.5.2
)

NO_LINT()
END()
