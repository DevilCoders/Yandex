INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.jdk.inc)

LINT(strict)

JAVAC_FLAGS(
    -Xlint:all
    -Xlint:-serial
    -Xlint:-processing
    -Xlint:-path
    -Xlint:-preview
)
JVM_ARGS(
)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/lombok.inc)

PEERDIR(
    contrib/java/org/jetbrains/annotations
)
