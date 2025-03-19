SELECT cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned
FROM dbaas.versions
WHERE cid = %(cid)s
      AND component = %(component)s
