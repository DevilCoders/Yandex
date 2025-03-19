USE hahn;
PRAGMA yt.InferSchema = '1';
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/support_knowladge_base/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

INSERT INTO {{output1}} WITH TRUNCATE 
SELECT
    `st_key`,
    `summary` || ' ' || `description` as `query`
FROM `//home/cloud/billing/exported-support-tables/tickets_prod` as x
LEFT ONLY JOIN `//home/cloud_analytics/ml/support_knowladge_base/train_embedings` as y
ON x.st_key == CAST(y.st_key as String)
LEFT ONLY JOIN `//home/cloud_analytics/ml/support_knowladge_base/look_a_like` as z
ON x.st_key == z.st_key