EXTERNAL_JAVA_LIBRARY()

OWNER(heretic)

EMBED_JAVA_VCS_INFO()

SRCS(
    src/main/java/ru/yandex/library/svnversion/VcsVersion.java
)

LINT(strict)

END()

RECURSE(
    tests
)
