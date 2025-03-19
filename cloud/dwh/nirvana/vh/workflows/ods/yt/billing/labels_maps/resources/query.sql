PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");


IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `helpers` SYMBOLS $to_str, $autoconvert_options;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


$labels_maps = (
    SELECT
        $to_str(billing_account_id)                         AS billing_account_id,
        labels_hash                                         AS labels_hash,
        cast(labels_json as JSON)                           AS labels_json,
        shard_id                                            AS shard_id
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    billing_account_id,
    labels_hash,
    labels_json,
    $to_str(JSON_VALUE(labels_json, "$.system_labels.folder_id"))           AS system_label_folder_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.cloud_id"))              AS user_label_cloud_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.cluster_id"))            AS user_label_cluster_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.folder_id"))             AS user_label_folder_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.subcluster_id"))         AS user_label_subcluster_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.subcluster_role"))       AS user_label_subcluster_role,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.hystax_backup_id"))      AS user_label_hystax_backup_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.hystax_device_id"))      AS user_label_hystax_device_id,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.hystax_device_name"))    AS user_label_hystax_device_name,
    $to_str(JSON_VALUE(labels_json, "$.user_labels.hystax_type"))           AS user_label_hystax_type,
    shard_id
FROM $labels_maps
ORDER BY `billing_account_id`,`labels_hash`, `shard_id`
