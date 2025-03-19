-- Please note that YQL schema not specified, so we're telling YQL to infer it from data.
PRAGMA yt.InferSchema = '1';
PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/support_knowladge_base/tmp';
use hahn;

$new=(
SELECT
    Yson::ConvertToDoubleList(`dst`) as `dst`,
    CAST(`st_key` as String) as `st_key`,
FROM {{input1}}
);

$old=(SELECT
    `dst` as `dst_old`,
    `st_key` as `st_key_old`,
FROM `//home/cloud_analytics/ml/support_knowladge_base/train_embedings`
);

$cross = (SELECT 
`st_key`,
`st_key_old`,
ListSum(ListMap(ListZipAll(`dst`, `dst_old`), ($x) -> { RETURN COALESCE($x.0, 0) * COALESCE($x.1, 0);})) as `sq_prod`
FROM $new as x
CROSS JOIN $old as y
);

INSERT INTO `//home/cloud_analytics/ml/support_knowladge_base/look_a_like`
SELECT * 
FROM
(SELECT 
    `st_key`,
    TOP_BY(`st_key_old`, `sq_prod`, 5) as `top_keys`,
    TOP_BY(`sq_prod`, `sq_prod`, 5) as `top_scores`
FROM $cross
GROUP BY `st_key`) as x
LEFT ONLY JOIN `//home/cloud_analytics/ml/support_knowladge_base/look_a_like` as y
ON x.st_key == y.st_key


