PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_date;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT helpers SYMBOLS $to_str, $lookup_bool, $lookup_string_list, $lookup_int64, $lookup_string, $autoconvert_options, $yson_to_uint64, $yson_to_str, $yson_to_list, $get_json_from_yson;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$toString = ($utf8) -> (CAST($utf8 AS String));

INSERT INTO $dst_table WITH TRUNCATE

SELECT *
FROM (
SELECT
    a.*,
    tags.0                          AS tag_level,
    tags.1                          AS tag_name
WITHOUT tags
FROM (
    SELECT
        $to_str(`name`)             AS schema_name,
        $to_str(`service_id`)       AS service_id,
        NVL(Yson::ConvertTo(tags,
                Dict<String,List <String?>?>?
                ),{cast(null as String?):cast([null] as List<String?>?)})
                                    AS tags
    FROM $select_transfer_manager_table($src_folder, $cluster)
    ) as a
FLATTEN DICT BY tags
) AS A
FLATTEN LIST BY tag_name
ORDER BY schema_name
