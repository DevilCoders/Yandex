USE_RECIPE(antiadblock/postgres_local/recipe/recipe --port 5432 --user postgres --db_name postgres)
INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)
DEPENDS(
  antiadblock/postgres_local/recipe
)
