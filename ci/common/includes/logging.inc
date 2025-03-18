PEERDIR (
    # используем slf4j как фасад
    contrib/java/org/slf4j/slf4j-api

    contrib/java/org/apache/logging/log4j/log4j-core
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/org/apache/logging/log4j/log4j-slf4j-impl

    # yaml configuration
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-yaml

    # байндим java.util.logging в slf4j. Позволяет избавиться от шума в интеграционных тестах
    # см. также JULBridgeExtension
    contrib/java/org/slf4j/jul-to-slf4j

    # Аппендер log4j в unified agent
    # https://a.yandex-team.ru/arc/trunk/arcadia/logbroker/unified_agent/client/java/log4j2/USAGE.md
    logbroker/unified_agent/client/java/log4j2
)

EXCLUDE (
    contrib/java/ch/qos/logback/logback-classic
)
