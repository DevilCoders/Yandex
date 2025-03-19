PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$convert_to_dict = ($yson) -> (IF ($yson is NULL or Yson::IsEntity($yson), NULL, Yson::ConvertTo($yson,Optional<Dict<String, Optional<Yson>>>)));
$convert_to_list = ($yson) -> (IF ($yson is NULL or Yson::IsEntity($yson), NULL, Yson::ConvertToList($yson)));

$result = (
    SELECT
        id                                                                                                      AS operation_id,
        type                                                                                                    AS type,
        status                                                                                                  AS status,
        reason                                                                                                  AS reason,
        created_by                                                                                              AS created_by,
        $get_datetime(created_at)                                                                               AS created_at,
        $get_datetime(modified_at)                                                                              AS modified_at,
        done                                                                                                    AS done,
        $convert_to_dict(error)                                                                                 AS error,
        $convert_to_dict(metadata)                                                                              AS metadata,

        -- good data looks like dict. Some dirty rows looks like list of 2 elements (dict + int)
        CASE WHEN response is NULL or Yson::IsEntity(response) THEN NULL
            WHEN Yson::IsDict(response) THEN $convert_to_dict(response)
            ELSE $convert_to_dict($convert_to_list(response)[0]) END                                            AS response
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY operation_id
