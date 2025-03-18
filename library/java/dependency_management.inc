SET(NETTY_VERSION 4.1.63.Final)
SET(SLF4J_VERSION 1.7.30)
SET(GUAVA_VERSION 30.1.1-jre)
SET(GRPC_NETTY_VERSION 1.43.2)
SET(JACKSON_VERSION 2.9.8)
SET(JUNIT_VERSION 4.12)
SET(NETTY_TCNATIVE_BORINGSSL_STATIC 2.0.31.Final)

DEPENDENCY_MANAGEMENT(
    contrib/java/junit/junit/${JUNIT_VERSION}

    contrib/java/io/netty/netty-all/${NETTY_VERSION}
    contrib/java/io/netty/netty-buffer/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-dns/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-haproxy/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-http/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-http2/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-memcache/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-mqtt/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-redis/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-smtp/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-socks/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-stomp/${NETTY_VERSION}
    contrib/java/io/netty/netty-codec-xml/${NETTY_VERSION}
    contrib/java/io/netty/netty-common/${NETTY_VERSION}
    contrib/java/io/netty/netty-handler/${NETTY_VERSION}
    contrib/java/io/netty/netty-handler-proxy/${NETTY_VERSION}
    contrib/java/io/netty/netty-resolver/${NETTY_VERSION}
    contrib/java/io/netty/netty-resolver-dns/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-native-epoll/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-native-epoll-linux-x86_64/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-native-kqueue/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-native-kqueue-osx-x86_64/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-native-unix-common/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-rxtx/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-sctp/${NETTY_VERSION}
    contrib/java/io/netty/netty-transport-udt/${NETTY_VERSION}

    contrib/java/org/apache/commons/commons-compress/1.20
    contrib/java/org/apache/commons/commons-csv/1.8
    contrib/java/org/apache/commons/commons-dbcp2/2.8.0
    contrib/java/org/apache/commons/commons-email/1.5
    contrib/java/org/apache/commons/commons-exec/1.3
    contrib/java/org/apache/commons/commons-io/1.3.2
    contrib/java/org/apache/commons/commons-jexl/2.1.1
    contrib/java/org/apache/commons/commons-jexl3/3.1
    contrib/java/org/apache/commons/commons-lang3/3.11
    contrib/java/org/apache/commons/commons-math/2.2
    contrib/java/org/apache/commons/commons-math3/3.6.1
    contrib/java/org/apache/commons/commons-pool2/2.9.0
    contrib/java/org/apache/commons/commons-text/1.9
    contrib/java/org/apache/commons/commons-vfs2/2.7.0

    contrib/java/org/slf4j/jcl-over-slf4j/${SLF4J_VERSION}
    contrib/java/org/slf4j/jul-to-slf4j/${SLF4J_VERSION}
    contrib/java/org/slf4j/log4j-over-slf4j/${SLF4J_VERSION}
    contrib/java/org/slf4j/slf4j-api/${SLF4J_VERSION}
    contrib/java/org/slf4j/slf4j-log4j12/${SLF4J_VERSION}
    contrib/java/org/slf4j/slf4j-simple/${SLF4J_VERSION}

    contrib/java/com/google/code/gson/gson/2.8.6
    contrib/java/com/google/guava/guava/${GUAVA_VERSION}
    contrib/java/it/unimi/dsi/fastutil/8.5.2

    contrib/java/org/liquibase/liquibase-core/3.8.0

    # jackson
    contrib/java/com/fasterxml/jackson/core/jackson-annotations/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/core/jackson-core/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/core/jackson-databind/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-jdk8/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-json-org/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-jsr310/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-csv/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-xml/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-yaml/${JACKSON_VERSION}
    contrib/java/com/fasterxml/jackson/module/jackson-module-jaxb-annotations/${JACKSON_VERSION}

    contrib/java/io/grpc/grpc-netty/${GRPC_NETTY_VERSION}

    contrib/java/io/netty/netty-tcnative-boringssl-static/${NETTY_TCNATIVE_BORINGSSL_STATIC}

    contrib/java/org/projectlombok/lombok/1.18.22
)

