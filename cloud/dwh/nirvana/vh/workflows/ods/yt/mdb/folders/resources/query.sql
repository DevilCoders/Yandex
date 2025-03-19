$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        folder_id                        AS folder_id,
        CAST(folder_ext_id AS String)    AS folder_ext_id,
        cloud_id                         AS cloud_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
