/*
show requests that CMS must handle
\c cmsdb
*/
\pset expanded on
\pset footer off
\pset columns 160
\pset format wrapped
SELECT r.fqdns[1] as host,
       to_char(NOW() - r.came_back_at, 'DD HH24H MIm') as came_back_ago,
       d.after_walle_log as resolution
FROM cms.requests r
LEFT JOIN cms.decisions d ON r.id = d.request_id
WHERE
      d.status = 'before-done'
ORDER BY came_back_at ASC;
