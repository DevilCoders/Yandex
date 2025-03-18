JDK_VERSION(18)

# TODO CI-2398
# Must be in sync with https://a.yandex-team.ru/arc_vcs/ci/common/application/src/main/script/application-start.sh?rev=faa495e29d#L72
JVM_ARGS(
    --add-opens=java.base/java.util=ALL-UNNAMED
    --add-opens=java.base/java.lang=ALL-UNNAMED
    --add-opens=java.base/java.time=ALL-UNNAMED
)
