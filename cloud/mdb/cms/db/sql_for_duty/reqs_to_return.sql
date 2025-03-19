/*
show requests that Wall-e should return
\c cmsdb
*/
SELECT r.request_ext_id,
       to_char(NOW() - r.resolved_at, 'DD HH24H MIm') as given_away,
       r.name as action,
       'https://wall-e.yandex-team.ru/host/' || r.fqdns[1] as host,
       to_char(r.created_at, 'DD Mon YYYY hh24:MI:SS TZ') as when_created
FROM cms.requests r
LEFT JOIN cms.decisions d ON r.id = d.request_id
WHERE resolved_at IS NOT NULL AND is_deleted = false
ORDER BY created_at ASC;
