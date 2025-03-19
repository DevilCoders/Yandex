#!/bin/bash
set -e -o pipefail

CLOUD_VERSION=${1}
#PROTO_VERSION=${2}

if [[ -z ${CLOUD_VERSION} ]]; then
    echo "usage ${0} <cloud-java.version>"
    exit 0
fi

DEPS=(
    # ru.yandex.cloud:cloud-proto-java:${PROTO_VERSION}
    "yandex.cloud.common.dependencies:jetty-application:${CLOUD_VERSION?}"
    "yandex.cloud.common.dependencies:jetty-application:jar:tests:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:application:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:lang-test:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:metrics:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:operation-client-test:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:repository-core-test:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:repository-core:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:repository-kikimr:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:scenario:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:static-di:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:task-processor:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:util:${CLOUD_VERSION?}"
    "yandex.cloud.common.library:util:jar:tests:${CLOUD_VERSION?}"
    "yandex.cloud:cloud-auth-config:${CLOUD_VERSION?}"
    # "yandex.cloud:cloud-auth-impl:${CLOUD_VERSION?}"
    "yandex.cloud:fake-cloud-core:${CLOUD_VERSION?}"
    "yandex.cloud:fake-cloud-core:jar:tests:${CLOUD_VERSION?}"
    "yandex.cloud:http-healthcheck:${CLOUD_VERSION?}"
    "yandex.cloud:iam-common-test:${CLOUD_VERSION?}"
    "yandex.cloud:iam-common:${CLOUD_VERSION?}"
    "yandex.cloud:iam-remote-operation:${CLOUD_VERSION?}"
    "yandex.cloud:operations:${CLOUD_VERSION?}"
)

ya maven-import --legacy-mode --checkout ${DEPS[@]}
# --remote-repository=https://artifactory.yandex.net/artifactory/public/,https://artifactory.yandex.net/artifactory/yandex_cloud/

# sed -i "s/^SET.cloud-proto\.version[^)]*/SET(cloud-proto.version ${PROTO_VERSION}/g" ya.dependency_management.inc
# sed -i "s/^SET.cloud-java\.version[^)]*/SET(cloud-java.version ${CLOUD_VERSION?}"/g" ya.dependency_management.inc
