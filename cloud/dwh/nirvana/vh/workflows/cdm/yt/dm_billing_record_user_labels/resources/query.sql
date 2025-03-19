-- Input
$billing_records_folder = {{ param["billing_records_folder"] -> quote() }};

-- Output
$destination_path = {{ param["destination_path"] -> quote() }};

$billing_record_user_labels = (
  SELECT
    br.labels_hash                                                                          AS labels_hash,
    Yson::LookupDict(Yson::ParseJson(SOME(br.labels_json), Yson::Options()), "user_labels") AS user_labels
  FROM RANGE($billing_records_folder) as br
  GROUP BY (br.labels_hash)
);

$billing_record_flatten_labels = (
  SELECT
    labels_hash                                             AS billing_record_labels_hash,
    `user_labels`.0                                         AS billing_record_user_label_key,
    Yson::ConvertToString(`user_labels`.1, Yson::Options()) AS billing_record_user_label_value
  FROM (
    SELECT
      *
    FROM $billing_record_user_labels
    FLATTEN DICT BY user_labels
  )
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  billing_record_labels_hash,
  billing_record_user_label_key,
  billing_record_user_label_value
FROM $billing_record_flatten_labels
ORDER BY billing_record_user_label_key, billing_record_user_label_value, billing_record_labels_hash;
