#Spring
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/springframework/boot/spring-boot-dependencies/2.6.6/ya.dependency_management.inc)

INCLUDE(${ARCADIA_ROOT}/contrib/java/io/netty/netty-bom/4.1.74.Final/ya.dependency_management.inc)

#Grpc and protobuf
INCLUDE(${ARCADIA_ROOT}/contrib/java/io/grpc/grpc-bom/1.46.0/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/com/google/protobuf/protobuf-bom/${JAVA_PROTO_RUNTIME_VERSION}/ya.dependency_management.inc)
DEPENDENCY_MANAGEMENT(
    contrib/java/io/opencensus/opencensus-api/0.28.0
    contrib/java/io/opencensus/opencensus-contrib-grpc-metrics/0.24.0
)

#Grpc rate limit interceptors
DEPENDENCY_MANAGEMENT(
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-core/0.3.6
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-grpc/0.3.6
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-spectator/0.3.6
    contrib/java/com/netflix/spectator/spectator-reg-micrometer/0.130.0
    contrib/java/com/netflix/spectator/spectator-api/0.130.0
)

#Common
DEPENDENCY_MANAGEMENT(
    contrib/java/com/google/guava/guava/31.1-jre
    contrib/java/com/google/code/gson/gson/2.9.0
    contrib/java/com/beust/jcommander/1.78
    contrib/java/org/json/json/20190722
    contrib/java/jakarta/validation/jakarta.validation-api/2.0.2
    contrib/java/org/apache/commons/commons-text/1.8
    contrib/java/com/github/java-json-tools/json-schema-validator/2.2.11
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    contrib/java/org/projectlombok/lombok/1.18.22
    contrib/java/one/util/streamex/0.7.2
    contrib/java/ru/yandex/passport/tvmauth-java/8897490
    contrib/java/io/burt/jmespath-gson/0.5.1-y
    contrib/java/me/dinowernli/java-grpc-prometheus/0.3.0
    contrib/java/jakarta/xml/bind/jakarta.xml.bind-api/2.3.3
    contrib/java/org/checkerframework/checker-qual/3.12.0
    contrib/java/com/amazonaws/aws-java-sdk-s3/1.11.983
    contrib/java/org/lz4/lz4-java/1.8.0
    contrib/java/org/yaml/snakeyaml/1.30
    contrib/java/com/google/auto/value/auto-value-annotations/1.9
)

#flow-engine
DEPENDENCY_MANAGEMENT(
    contrib/java/cglib/cglib-nodep/2.2
    contrib/java/com/google/j2objc/j2objc-annotations/1.3
    contrib/java/com/trilead/trilead-ssh2/1.0.0-build221
    contrib/java/commons-collections/commons-collections/3.2.2
    contrib/java/org/reflections/reflections/0.9.11
)

#Test
DEPENDENCY_MANAGEMENT(
    contrib/java/org/easymock/easymock/3.4
    contrib/java/org/mock-server/mockserver-netty/5.8.1
    contrib/java/org/mock-server/mockserver-junit-jupiter/5.8.1
    contrib/java/io/github/benas/random-beans/3.9.0
)

#Temporal
DEPENDENCY_MANAGEMENT(
    contrib/java/io/temporal/temporal-sdk/1.11.0
    contrib/java/io/temporal/temporal-testing/1.11.0
    contrib/java/com/cronutils/cron-utils/9.1.6
)
EXCLUDE(
    #Temporal requires newer libs than we want
    contrib/java/org/slf4j/slf4j-api/2.0.0-alpha7
    contrib/java/io/micrometer/micrometer-core/1.8.5
)


#Cloud-java
DEPENDENCY_MANAGEMENT(
    contrib/java/yandex/cloud/common/library/repository-kikimr/26158-3da4459e71
    contrib/java/yandex/cloud/common/dependencies/tracing/26158-3da4459e71
    contrib/java/com/google/api/grpc/proto-google-common-protos/2.7.1
    contrib/java/com/squareup/okio/okio/2.8.0
    contrib/java/org/apache/commons/commons-collections4/4.4
    contrib/java/org/bouncycastle/bcprov-jdk15on/1.69
    contrib/java/com/squareup/okhttp3/okhttp/4.9.0
    contrib/java/org/bouncycastle/bcpkix-jdk15on/1.67
)


#Retrofit https://github.com/square/retrofit/issues/3231
DEPENDENCY_MANAGEMENT(
    contrib/java/com/squareup/retrofit2/retrofit/2.9.0
    contrib/java/com/squareup/retrofit2/converter-jackson/2.9.0
    contrib/java/org/asynchttpclient/async-http-client-extras-retrofit2/2.12.3
    contrib/java/org/asynchttpclient/async-http-client/2.12.3
)


# Using log4j2
EXCLUDE(
    contrib/java/org/springframework/boot/spring-boot-starter-logging
    contrib/java/org/slf4j/slf4j-log4j12
    contrib/java/commons-logging/commons-logging
    contrib/java/log4j/log4j
    contrib/java/org/junit/vintage/junit-vintage-engine
    contrib/java/org/slf4j/slf4j-simple
)


# Using JUnit5
EXCLUDE(
    contrib/java/junit/junit
)


# Misc excludes
EXCLUDE(
    contrib/java/org/hamcrest/hamcrest-all

    # Conflict with mockserver-core/5.8.1
    contrib/java/javax/xml/bind/jaxb-api/2.4.0-b180830.0359

    # Error[-WMisconfiguration]: in $B/ci/tools/java/ci-tools: More recent libraries versions in PEERDIRs: 3.0.0-M5 (contrib/java/org/glassfish/jaxb/jaxb-runtime/2.3.3 required by DEPENDENCY_MANAGEMENT)
    contrib/java/org/glassfish/jaxb/jaxb-runtime/3.0.0-M5

    contrib/java/javax/activation/activation
    contrib/java/javax/xml/bind/jaxb-api
    contrib/java/javax/activation/javax.activation-api

    contrib/java/com/sun/activation/javax.activation
    contrib/java/com/sun/activation/jakarta.activation
)

#Curator https://curator.apache.org/zk-compatibility-34.html
DEPENDENCY_MANAGEMENT(
    contrib/java/org/apache/curator/curator-recipes/4.3.0
    contrib/java/org/apache/curator/curator-test/4.3.0
    contrib/java/org/apache/zookeeper/zookeeper/3.5.7
)

#Misc fixes for JAVA_DEPENDENCIES_CONFIGURATION(FORBID_CONFLICT)
DEPENDENCY_MANAGEMENT(
    contrib/java/com/google/errorprone/error_prone_annotations/2.11.0
    contrib/java/commons-io/commons-io/2.11.0
    contrib/java/org/objenesis/objenesis/3.2
    contrib/java/xml-apis/xml-apis/1.4.01
    contrib/java/org/ow2/asm/asm/9.2
    contrib/java/net/sf/jopt-simple/jopt-simple/5.0.3
    contrib/java/org/javassist/javassist/3.28.0-GA
    contrib/java/com/github/luben/zstd-jni/1.4.3-1
    contrib/java/joda-time/joda-time/2.10.9
    contrib/java/ru/yandex/clickhouse/clickhouse-jdbc/0.3.2
)

EXCLUDE(
    # Don't use all-in-one-bundle
    contrib/java/io/netty/netty-all
    contrib/java/org/ow2/asm/asm-all
)

#Mongo deps for bazinga, remove after CI-2204
DEPENDENCY_MANAGEMENT(
    contrib/java/org/mongodb/bson/3.11.2
)

EXCLUDE(
    contrib/java/org/mongodb/mongodb-driver
)

#Misc garbage from iceberg
EXCLUDE(
    contrib/java/mysql/mysql-connector-java
    contrib/java/org/postgresql/postgresql
)


#JMH
DEPENDENCY_MANAGEMENT(
    contrib/java/org/openjdk/jmh/jmh-core/1.34
    contrib/java/org/openjdk/jmh/jmh-generator-annprocess/1.34
    contrib/java/net/sf/jopt-simple/jopt-simple/5.0.4
)


EXCLUDE(
    # Excluding classpath duplicates
    # https://github.com/spring-projects/spring-boot/wiki/Spring-Boot-2.2-Release-Notes#jakarta-ee-dependencies
    contrib/java/javax/websocket/javax.websocket-api
    # use contrib/java/jakarta/servlet/jakarta.servlet-api
    contrib/java/javax/annotation/javax.annotation-api
    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/javax/servlet/servlet-api
    contrib/java/javax/xml/bind/jaxb-api
    contrib/java/javax/websocket/javax.websocket-client-api
    # use contrib/java/jakarta/annotation/jakarta.annotation-api
    contrib/java/org/glassfish/hk2/external/jakarta.inject
    # use contrib/java/jakarta/validation/jakarta.validation-api
    contrib/java/javax/validation/validation-api
    # use contrib/java/jakarta/xml/bind/jakarta.xml.bind-api
    contrib/java/javax/xml
    contrib/java/javax/xml/bind
    contrib/java/javax/xml/bind/jaxb-api
    contrib/java/javax/activation
    contrib/java/javax/activation/javax.activation-api
    contrib/java/org/apache/tomcat/tomcat-annotations-api
    contrib/java/org/apache/tomcat/embed/tomcat-embed-core

    # Using Jetty instead of Tomcat
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/org/apache/tomcat/embed/tomcat-embed-el

    # Conflict with library versions because kikimr-repository
    contrib/java/com/yandex/ydb/ydb-sdk-proto
    contrib/java/com/yandex/ydb/ydb-sdk-table
    contrib/java/com/yandex/ydb/ydb-sdk-core

    # Useless deps
    iceberg/inside-solomon
    iceberg/inside-juggler
    # iceberg/inside-passport # Required by Bazinga Web Admin
    iceberg/commune-a3-demo
    iceberg/commune-graphite
    iceberg/commune-mongo
    iceberg/misc/testlib

    # Jetbrains
    contrib/java/org/jetbrains/annotations
)
