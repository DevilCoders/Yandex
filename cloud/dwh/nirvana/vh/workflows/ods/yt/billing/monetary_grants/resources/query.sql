PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$convert_to_dict = ($yson) -> (IF ($yson is NULL or Yson::IsEntity($yson), NULL, Yson::ConvertToDict($yson)));
$lookup_from_dict = ($dict, $key) -> (DictLookup($convert_to_dict($dict), $key));

$result = (
    SELECT
        billing_account_id,
        id,
        $get_datetime(start_time) as start_time,
        $get_datetime(created_at) as created_at,
        $get_datetime(end_time) as end_time,
        cast (initial_amount as Double) as initial_amount,
        source,
        source_id,
        $lookup_from_dict(filter_info, "category")                                              AS filter_category,
        $lookup_from_dict(filter_info, "level")                                                 AS filter_level,
        $lookup_from_dict(filter_info, "entity_ids")                                            AS filter_entity_ids,
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY billing_account_id, start_time
