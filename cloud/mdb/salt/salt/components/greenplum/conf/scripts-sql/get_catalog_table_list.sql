SELECT b.nspname AS table_schema, a.relname AS table_name
FROM pg_class a, pg_namespace b
WHERE a.relnamespace=b.oid
  AND b.nspname='pg_catalog'
  AND a.relkind='r'
