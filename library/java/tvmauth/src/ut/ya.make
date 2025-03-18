JTEST()

JDK_VERSION(11)

OWNER(g:passport_infra)

PEERDIR(
    contrib/java/junit/junit/4.12
    contrib/java/org/slf4j/slf4j-simple
    devtools/jtest
    library/java/tvmauth
)

JAVA_SRCS(
    SRCDIR
    ${ARCADIA_ROOT}/library/java/tvmauth/src/test
    **/*
)

JAVA_SRCS(simplelogger.properties)

DATA(arcadia/library/cpp/tvmauth/client/ut/files)

LINT(extended)

# Added automatically to remove dependency on default contrib versions
DEPENDENCY_MANAGEMENT(
    contrib/java/org/slf4j/slf4j-simple/1.8.0-alpha2
)

# tirole
INCLUDE(${ARCADIA_ROOT}/library/recipes/tirole/recipe.inc)
USE_RECIPE(
    library/recipes/tirole/tirole
    --roles-dir library/java/tvmauth/src/ut/roles
)

# tvmapi - to provide service ticket for tirole
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

# tvmtool
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmtool/recipe.inc)
USE_RECIPE(
    library/recipes/tvmtool/tvmtool
    library/java/tvmauth/src/ut/tvmtool.cfg
    --with-roles-dir library/java/tvmauth/src/ut/roles
)

END()
