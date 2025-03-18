BUILD_ONLY_IF(OS_LINUX ARCH_X86_64)

DATA(sbr://678806418)  # postgres

DEPENDS(library/recipes/postgresql)

IF (NOT DEFINED PG_MAX_CONNECTIONS)
    SET(PG_MAX_CONNECTIONS 20)
ENDIF()

IF (DEFINED PG_MIGRATIONS_DIR)
    IF (DEFINED PG_SCHEMA_MIGRATIONS_DIR)
        USE_RECIPE(library/recipes/postgresql/postgresql_recipe -m ${PG_MIGRATIONS_DIR} -s ${PG_SCHEMA_MIGRATIONS_DIR} -n ${PG_MAX_CONNECTIONS})
    ELSE()
        USE_RECIPE(library/recipes/postgresql/postgresql_recipe -m ${PG_MIGRATIONS_DIR} -n ${PG_MAX_CONNECTIONS})
    ENDIF()
ELSE()
    IF (DEFINED PG_SCHEMA_MIGRATIONS_DIR)
        USE_RECIPE(library/recipes/postgresql/postgresql_recipe -s ${PG_SCHEMA_MIGRATIONS_DIR} -n ${PG_MAX_CONNECTIONS})
    ELSE()
        USE_RECIPE(library/recipes/postgresql/postgresql_recipe -n ${PG_MAX_CONNECTIONS})
    ENDIF()
ENDIF()
