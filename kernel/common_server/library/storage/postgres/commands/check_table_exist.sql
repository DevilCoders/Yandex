SELECT COUNT(*)
FROM pg_class
WHERE relname='$TABLE_NAME$' AND relkind='r';