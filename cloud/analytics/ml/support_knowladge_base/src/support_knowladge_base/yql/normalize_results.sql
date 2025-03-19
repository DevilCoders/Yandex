use hahn;
PRAGMA yt.InferSchema = '1';
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/support_knowladge_base/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';
$div = ($y) -> {
    return ($x) -> { RETURN $x/$y; };
};

$table = (
SELECT
    Yson::ConvertToDoubleList(`dst`) as `dst`,
    Math::Sqrt(ListSum(ListMap(Yson::ConvertToDoubleList(`dst`), ($x) -> { RETURN $x * $x; }))) as `sq_sum`,
    CAST(`st_key` as String) as `st_key`,
FROM {{input1}}
);

INSERT INTO `//home/cloud_analytics/ml/support_knowladge_base/train_embedings` WITH TRUNCATE
SELECT 
    `st_key`,
    ListMap(`dst`, $div(`sq_sum`)) as `dst`
FROM $table 