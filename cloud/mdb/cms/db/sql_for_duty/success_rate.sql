/*
success rate of CMS
\c cmsdb
*/
SELECT to_char(date_trunc('week', r.resolved_at), 'DD Mon YYYY')                         as week,
       COUNT(*) filter (where r.resolved_by = 'robot-mdb-cms-porto') as auto,
       COUNT(*)                                                                          as total,
       100 * round(cast(COUNT(*) filter (where r.resolved_by = 'cms autoduty') as decimal) / COUNT(*), 2) as auto_percent
FROM cms.requests r
WHERE r.resolved_at IS NOT NULL
GROUP BY week
ORDER BY week DESC;
