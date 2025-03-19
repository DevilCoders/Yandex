CHECK_JAVA_DEPS(strict)
JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    REQUIRE_DM
)

SET(access-service-api-proto.version 1.0.20220316-14)
SET(access-service-client.version 1.0.20220317-18)
SET(assertj.version 3.22.0)
SET(cloud-java.version 26849-91120c3706)
SET(cloud-proto.version 1.0.21327-bb65b3cb97)
SET(commons-codec.version 1.15)
SET(commons-io.version 2.11.0)
SET(commons-lang3.version 3.9)
SET(commons-pool2.version 2.6.2)
SET(commons-text.version 1.8)
SET(grpc.version 1.43.2)  # required by cloud-java code, using 1.44.0 causes java.lang.ClassNotFoundException: io.grpc.ClientStreamTracer$InternalLimitedInfoFactory
SET(guava.version 31.0.1-jre)
SET(httpcore.version 4.4.14)
SET(jackson.version 2.12.3)
SET(javax.annotation-api.version 1.3.2)
SET(javax.servlet-api.version 4.0.1)
SET(jetbrains-annotations.version 20.1.0)
SET(jetty.version 9.4.45.v20220203)
SET(junit.version 4.13.2)
SET(log4j2.version 2.17.1)
SET(lombok.version 1.18.20)
SET(mockito.version 3.9.0)
SET(netty.version 4.1.72.Final)
SET(okhttp3.version 4.9.2)
SET(prometheus.version 0.6.0)
SET(proto-google-common-protos.version 2.7.1)
SET(protobuf.version ${JAVA_PROTO_RUNTIME_VERSION}) # arcadia current ${JAVA_PROTO_RUNTIME_VERSION}=3.18.2?
SET(resteasy.version 3.9.0.Final)
SET(slf4j.version 1.7.32)

INCLUDE(${ARCADIA_ROOT}/contrib/java/com/fasterxml/jackson/jackson-bom/${jackson.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/com/google/protobuf/protobuf-bom/${protobuf.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/com/squareup/okhttp3/okhttp-bom/${okhttp3.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/io/grpc/grpc-bom/${grpc.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/io/netty/netty-bom/${netty.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/apache/logging/log4j/log4j-bom/${log4j2.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/eclipse/jetty/jetty-bom/${jetty.version}/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/jboss/resteasy/resteasy-bom/${resteasy.version}/ya.dependency_management.inc)

DEPENDENCY_MANAGEMENT(
    contrib/java/com/google/api/grpc/proto-google-common-protos/${proto-google-common-protos.version}
    contrib/java/com/google/guava/guava/${guava.version}
    contrib/java/commons-codec/commons-codec/${commons-codec.version}
    contrib/java/commons-io/commons-io/${commons-io.version}
    contrib/java/io/prometheus/simpleclient/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_caffeine/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_common/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_guava/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_hotspot/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_httpserver/${prometheus.version}
    contrib/java/io/prometheus/simpleclient_log4j2/${prometheus.version}
    contrib/java/javax/annotation/javax.annotation-api/${javax.annotation-api.version}
    contrib/java/javax/inject/javax.inject/1
    contrib/java/javax/servlet/javax.servlet-api/${javax.servlet-api.version}
    contrib/java/junit/junit/${junit.version}
    contrib/java/org/apache/commons/commons-lang3/${commons-lang3.version}
    contrib/java/org/apache/commons/commons-pool2/${commons-pool2.version}
    contrib/java/org/apache/commons/commons-text/${commons-text.version}
    contrib/java/org/apache/httpcomponents/httpcore/${httpcore.version}
    contrib/java/org/assertj/assertj-core/${assertj.version}
    contrib/java/org/jetbrains/annotations/${jetbrains-annotations.version}
    contrib/java/org/mockito/mockito-core/${mockito.version}
    contrib/java/org/projectlombok/lombok/${lombok.version}
    contrib/java/org/slf4j/slf4j-api/${slf4j.version}
    contrib/java/ru/yandex/cloud/cloud-proto-java/${cloud-proto.version}
    contrib/java/yandex/cloud/cloud-auth-config/${cloud-java.version}
    contrib/java/yandex/cloud/cloud-auth-i18n-impl/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/grpc/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/http-server/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/jetty-application-tests/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/jetty-application/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/log/${cloud-java.version}
    contrib/java/yandex/cloud/common/dependencies/tracing/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/application/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/business-events/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/call-api-core/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/data-binding/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/i18n/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/json-util/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/lang-test/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/lang/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/log-util/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/metrics/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/operation-client-test/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/repository-core-test/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/repository-core/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/repository-kikimr/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/resource-manager-client/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/scenario/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/static-di/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/task-processor/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/util-tests/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/util/${cloud-java.version}
    contrib/java/yandex/cloud/common/library/validation/${cloud-java.version}
    contrib/java/yandex/cloud/events/${cloud-java.version}
    contrib/java/yandex/cloud/fake-auth-testlib/${cloud-java.version}
    contrib/java/yandex/cloud/fake-cloud-core-tests/${cloud-java.version}
    contrib/java/yandex/cloud/fake-cloud-core/${cloud-java.version}
    contrib/java/yandex/cloud/http-healthcheck/${cloud-java.version}
    contrib/java/yandex/cloud/iam-common-exception/${cloud-java.version}
    contrib/java/yandex/cloud/iam-common-test/${cloud-java.version}
    contrib/java/yandex/cloud/iam-common-tvm/${cloud-java.version}
    contrib/java/yandex/cloud/iam-common/${cloud-java.version}
    contrib/java/yandex/cloud/iam-remote-operation/${cloud-java.version}
    contrib/java/yandex/cloud/iam/access-service-api-proto/${access-service-api-proto.version}
    contrib/java/yandex/cloud/iam/access-service-client-api/${access-service-client.version}
    contrib/java/yandex/cloud/iam/access-service-client-grpc-api/${access-service-client.version}
    contrib/java/yandex/cloud/iam/access-service-client-impl/${access-service-client.version}
    contrib/java/yandex/cloud/iam/access-service-client-token/${access-service-client.version}
    contrib/java/yandex/cloud/operations/${cloud-java.version}
    contrib/java/yandex/cloud/quota/${cloud-java.version}
)

EXCLUDE(
    contrib/java/com/google/protobuf/protobuf-javalite # leaks from contrib/java/io/grpc/grpc-protobuf-lite, which leaks from contrib/java/io/grpc/grpc-protobuf
    contrib/java/com/sun/activation/javax.activation
    contrib/java/com/yandex/cloud/java-genproto
    contrib/java/javax/servlet/servlet-api  # replaced with contrib/java/javax/servlet/javax.servlet-api
    contrib/java/org/apache/tomcat  # leaks from contrib/java/yandex/cloud/common/dependencies/tracing
    contrib/java/org/jboss/spec/javax/annotation/jboss-annotations-api_1.3_spec
    contrib/java/org/slf4j/slf4j-log4j12
    contrib/java/ru/yandex/library-ticket_parser2-java
    contrib/java/yandex/cloud/search  # unused, leaks from iam-common, brings a lot of classpath-clashing dependencies
)
