DO $$
BEGIN
IF 'p' <> (SELECT contype FROM _$TABLE_NAME$_pkey) OR
   (SELECT attname FROM _$TABLE_NAME$_pkey) <> ARRAY['key'] THEN
    ALTER TABLE $TABLE_NAME$ DROP CONSTRAINT IF EXISTS $TABLE_NAME$_pkey CASCADE;
    ALTER TABLE $TABLE_NAME$ ADD PRIMARY KEY (key);
END IF;
END
$$
