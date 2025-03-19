use hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/support_knowladge_base/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

$main = (SELECT 
    `st_key`,
    Digest::MurMurHash32(`st_key`) as `hash`,
    `query`,
    ListConcat(AGGREGATE_LIST(`text`), ' ') as `doc`
FROM `//home/cloud_analytics/ml/support_tickets_classification/comment_templates/all_outgoing_comments`
WHERE `state` == 'closed'
AND NOT (COALESCE(`components`, '') LIKE '%квоты%')
GROUP BY `st_key`, `summary` || ' ' || `description` as `query`);

INSERT INTO {{output1}} WITH TRUNCATE
SELECT 
    `st_key`,
    `query`,
    `doc`
FROM $main 
WHERE `hash` % 5 != 0;

INSERT INTO {{output2}} WITH TRUNCATE
SELECT 
    `st_key`,
    `query`,
    `doc`
FROM $main 
WHERE `hash` % 5 == 0