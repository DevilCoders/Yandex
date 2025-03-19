DO $$
BEGIN
  IF NOT EXISTS (SELECT * FROM pg_class WHERE relname='$ADDITIONAL_TABLE_NAME$') THEN
    CREATE TABLE $ADDITIONAL_TABLE_NAME$ AS
      SELECT key, max(version) as last_version
      FROM $TABLE_NAME$
      GROUP BY key;

    ALTER TABLE $ADDITIONAL_TABLE_NAME$ ADD PRIMARY KEY(key);

    ALTER TABLE $ADDITIONAL_TABLE_NAME$ ADD FOREIGN KEY (key, last_version) REFERENCES $TABLE_NAME$(key, version);

  ELSE
    CREATE TEMP TABLE _$ADDITIONAL_TABLE_NAME$_join (relname name, conname name, contype char, attname text[]) ON COMMIT DROP;

    INSERT INTO _$ADDITIONAL_TABLE_NAME$_join
    SELECT relname, conname, contype, array_agg(attname ORDER BY attname) as attnames
    FROM(
      SELECT conname, contype, conkey, pg_class.oid as reloid, relname
      FROM pg_constraint
      JOIN pg_class
      ON conrelid=pg_class.oid
    ) AS tmp
    JOIN pg_attribute
    ON reloid=attrelid and attnum = ANY(conkey)
    WHERE relname='$ADDITIONAL_TABLE_NAME$'
    GROUP BY (relname, conname, contype);

    CREATE TEMP TABLE _$ADDITIONAL_TABLE_NAME$_pkey (relname name, conname name, contype char, attname text[]) ON COMMIT DROP;
    
    INSERT INTO _$ADDITIONAL_TABLE_NAME$_pkey
    SELECT *
    FROM _$ADDITIONAL_TABLE_NAME$_join
    WHERE conname='$ADDITIONAL_TABLE_NAME$_pkey';

    IF 'p' <> (SELECT contype FROM _$ADDITIONAL_TABLE_NAME$_pkey) OR
       (SELECT attname FROM _$ADDITIONAL_TABLE_NAME$_pkey) <> ARRAY['key'] THEN
        ALTER TABLE $ADDITIONAL_TABLE_NAME$ DROP CONSTRAINT IF EXISTS $ADDITIONAL_TABLE_NAME$_pkey CASCADE;
        ALTER TABLE $ADDITIONAL_TABLE_NAME$ ADD PRIMARY KEY (key);
    END IF;

    CREATE TEMP TABLE _$ADDITIONAL_TABLE_NAME$_fkey (relname name, conname name, contype char, attname text[]) ON COMMIT DROP;

    INSERT INTO _$ADDITIONAL_TABLE_NAME$_fkey
    SELECT *
    FROM _$ADDITIONAL_TABLE_NAME$_join
    WHERE conname='$ADDITIONAL_TABLE_NAME$_key_fkey';

    IF 'f' <> (SELECT contype FROM _$ADDITIONAL_TABLE_NAME$_fkey) OR
       (SELECT attname FROM _$ADDITIONAL_TABLE_NAME$_fkey) <> ARRAY['key', 'last_version'] THEN
        ALTER TABLE $ADDITIONAL_TABLE_NAME$ DROP CONSTRAINT IF EXISTS $ADDITIONAL_TABLE_NAME$_key_fkey CASCADE;
        ALTER TABLE $ADDITIONAL_TABLE_NAME$ ADD FOREIGN KEY (key, last_version) REFERENCES $TABLE_NAME$(key, version);
    END IF;
  END IF;
END;
$$
