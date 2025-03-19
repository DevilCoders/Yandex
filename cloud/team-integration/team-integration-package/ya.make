OWNER(g:cloud-iam)

JAVA_PROGRAM(team-integration-package)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.jdk.inc)

PEERDIR(
    cloud/team-integration/team-integration-application
)

INCLUDE(${ARCADIA_ROOT}/contrib/java/com/google/protobuf/protobuf-bom/${JAVA_PROTO_RUNTIME_VERSION}/ya.dependency_management.inc)

UBERJAR()
UBERJAR_SERVICES_RESOURCE_TRANSFORMER()
UBERJAR_MANIFEST_TRANSFORMER_MAIN(yandex.cloud.team.integration.application.Application)

UBERJAR_PATH_EXCLUDE_PREFIX(
    META-INF/*.SF
    META-INF/*.RSA
    META-INF/*.DSA
    **/Log4j2Plugins.dat
)

END()
