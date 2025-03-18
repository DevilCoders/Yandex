JAVA_PROGRAM(metrics-pull)

JDK_VERSION(11)
MAVEN_GROUP_ID(ru.yandex.monlib.examples)

OWNER(g:solomon)

SET(reactor_netty_version 0.9.10.RELEASE)
SET(spring_version 5.2.8.RELEASE)
SET(slf4j_version 1.8.0-alpha2)
SET(log4j_version 2.11.0)
SET(swagger_version 3.0.0)
SET(jackson_version 2.9.10)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.example.pull
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
    library/java/monlib/metrics-examples/metrics-data-model
    library/java/monlib/metrics-webflux
    # 3rd-party dependency
    contrib/java/org/slf4j/jul-to-slf4j/${slf4j_version}
    contrib/java/org/apache/logging/log4j/log4j-slf4j-impl/${log4j_version}

    contrib/java/org/springframework/spring-context/${spring_version}
    contrib/java/org/springframework/spring-core/${spring_version}
    contrib/java/org/springframework/spring-web/${spring_version}
    contrib/java/org/springframework/spring-webmvc/${spring_version}
    contrib/java/org/springframework/spring-webflux/${spring_version}

    contrib/java/com/fasterxml/jackson/core/jackson-core/${jackson_version}
    contrib/java/com/fasterxml/jackson/core/jackson-databind/${jackson_version}
    contrib/java/com/fasterxml/jackson/core/jackson-annotations/${jackson_version}

    contrib/java/io/springfox/springfox-swagger2/${swagger_version}
    contrib/java/io/projectreactor/netty/reactor-netty/${reactor_netty_version}
)

LINT(base)
END()
