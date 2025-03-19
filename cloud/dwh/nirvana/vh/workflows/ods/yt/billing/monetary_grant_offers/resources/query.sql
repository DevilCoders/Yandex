PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$convert_to_dict = ($yson) -> (IF ($yson is NULL or Yson::IsEntity($yson), NULL, Yson::ConvertToDict($yson)));

$lookup_from_dict = ($dict, $key) -> (DictLookup($convert_to_dict($dict), $key));

$lookup_string = ($yson, $key) -> (String::Strip(Yson::ConvertToString($lookup_from_dict($yson, $key))));


$result = (
    SELECT
        id                                                                                      AS monetary_grant_offer_id,
        $get_msk_datetime(created_at)                                                           AS created_at_msk,
        duration                                                                                AS duration,
        $to_decimal_35_9(initial_amount)                                                        AS initial_amount,
        passport_uid                                                                            AS passport_user_id,
        proposed_to                                                                             AS proposed_to,
        NVL($lookup_string(proposed_meta, "reason"), $lookup_string(proposed_meta, "ticket"))   AS reason,
        $lookup_string(proposed_meta, "staffLogin")                                             AS created_by,
        $get_msk_datetime(expiration_time)                                                      AS expiration_msk_time,
        $lookup_from_dict(filter_info, "category")                                              AS filter_category,
        $lookup_from_dict(filter_info, "level")                                                 AS filter_level,
        $lookup_from_dict(filter_info, "entity_ids")                                            AS filter_entity_ids,
        currency                                                                                AS currency
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY monetary_grant_offer_id
