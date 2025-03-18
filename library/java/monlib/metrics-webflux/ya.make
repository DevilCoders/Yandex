JAVA_LIBRARY(metrics-webflux)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

SET(spring_version 5.2.8.RELEASE)
SET(slf4j_version 1.8.0-alpha2)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.webflux
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
    library/java/monlib/metrics-http
    # 3rd-party dependency
    contrib/java/org/slf4j/jul-to-slf4j/${slf4j_version}

    contrib/java/org/springframework/spring-context/${spring_version}
    contrib/java/org/springframework/spring-webflux/${spring_version}
)

LINT(base)
END()

RECURSE_FOR_TESTS(ut)
