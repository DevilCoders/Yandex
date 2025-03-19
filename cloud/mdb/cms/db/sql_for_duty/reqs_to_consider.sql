/*
show requests to manage hosts
\c cmsdb
*/
\pset expanded on
\pset footer off
\pset columns 160
\pset format wrapped
SELECT r.name as action,
       r.fqdns[1] as host,
       'https://wall-e.yandex-team.ru/host/' || r.fqdns[1] as link_to_walle,
       r.request_type,
       r.request_ext_id as request_id,
       r.author,
       to_char(r.created_at, 'DD Mon YYYY hh24:MI:SS TZ') as when_created,
       r.comment as comment_by_walle,
       d.explanation as analysis_log,
       d.ad_resolution as autoduty_decision,
       d.mutations_log as let_go_log,
       d.status as decision_state
FROM cms.requests r
LEFT JOIN cms.decisions d ON r.id = d.request_id
WHERE resolved_at IS NULL AND is_deleted = false
ORDER BY created_at ASC;
