CREATE TABLE IF NOT EXISTS $TABLE_NAME$ (key varchar(200) PRIMARY KEY, value text);

CREATE TEMP TABLE _$TABLE_NAME$_join (relname name, conname name, contype char, attname text[]) ON COMMIT DROP;
INSERT INTO _$TABLE_NAME$_join
SELECT relname, conname, contype, array_agg(attname ORDER BY attname) AS attnames
FROM (
  SELECT conname, contype, conkey, pg_class.oid AS reloid, relname
  FROM pg_constraint
  JOIN pg_class
  ON conrelid=pg_class.oid
) AS tmp
JOIN pg_attribute
ON reloid=attrelid AND attnum = ANY(conkey)
WHERE relname='$TABLE_NAME$'
GROUP BY (relname, conname, contype);

CREATE TEMP TABLE _$TABLE_NAME$_pkey (relname name, conname name, contype char, attname text[]) ON COMMIT DROP;
INSERT INTO _$TABLE_NAME$_pkey
SELECT * FROM _$TABLE_NAME$_join WHERE conname='$TABLE_NAME$_pkey';