SELECT COUNT(*)
FROM pg_class
WHERE (relname='$ADDITIONAL_TABLE_NAME$' AND relkind='r') OR (relname='$TABLE_NAME$_version_seq' AND relkind='S') OR (relname='$TABLE_NAME$' AND relkind='r');