JAVA_LIBRARY(ci-engine)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

IDEA_RESOURCE_DIRS(src/main/resources/local)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/calendar
    ci/clients/ci
    ci/clients/observer
    ci/clients/staff
    ci/clients/storage
    ci/clients/testenv
    ci/clients/xiva
    ci/clients/yav

    ci/internal/ci/flow-engine
    ci/internal/common/logbroker

    ci/proto/event
    ci/proto/autocheck
    ci/proto/pciexpress

    repo/pciexpress/proto
    devtools/distbuild/pool_config_manager/proto_api

    contrib/java/org/apache/commons/commons-text
    contrib/java/jakarta/xml/bind/jakarta.xml.bind-api

    # Temporary, until moving to Java tasklets
    ci/tasklet/registry/common/ci/run_ci_action/proto
    ci/tasklet/registry/common/misc/sleep/proto
    ci/tasklet/registry/common/tracker/create_issue/proto
    ci/tasklet/registry/common/tracker/transit_issue/proto
    ci/tasklet/registry/common/tracker/update_issue/proto
    ci/tasklet/registry/common/tracker/migration/proto
    ci/clients/tracker

)


END()

RECURSE_FOR_TESTS(
    src/test
)
