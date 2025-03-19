use hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/support_knowladge_base/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';
INSERT INTO {{output1}} WITH TRUNCATE
SELECT
    `st_key`,
    `summary` || ' ' || `description` as `query`
FROM `//home/cloud/billing/exported-support-tables/tickets_prod`