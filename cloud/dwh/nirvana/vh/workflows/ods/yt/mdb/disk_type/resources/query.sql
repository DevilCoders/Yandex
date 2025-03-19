$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
    disk_type_id                         AS disk_type_id,
    CAST(disk_type_ext_id AS String)     AS disk_type_ext_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
