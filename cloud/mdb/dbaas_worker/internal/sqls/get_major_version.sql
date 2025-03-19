SELECT major_version
FROM dbaas.versions
WHERE cid = %(cid)s
      AND component = %(component)s
