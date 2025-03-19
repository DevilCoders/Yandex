WITH def_allocs AS (
  SELECT di.generation AS gen, geo,
         ROUND(CAST(AVG(c.cpu_limit) AS NUMERIC), 2) AS cpu_avg,
         ROUND(CAST(AVG(c.memory_limit / power(2,30)) AS NUMERIC), 2) AS memory_avg,
         ROUND(CAST(AVG(v.space_limit / power(2,30)) AS NUMERIC), 2) AS ssd_space_avg
  FROM mdb.containers AS c
  JOIN mdb.dom0_info AS di ON c.dom0 = di.fqdn AND di.project = 'pgaas'
  JOIN mdb.volumes AS v ON c.fqdn = v.container AND v.path != '/' AND v.disk_id IS NULL
  WHERE c.delete_token IS NULL
  AND di.heartbeat > now()::DATE - 1
  GROUP BY di.generation, geo

)

SELECT di.fqdn,
       i.geo,
       i.gen,
       CASE WHEN CAST(di.free_cores AS NUMERIC)/di.total_cores > 0.2 THEN di.free_cores / i.cpu_avg ELSE 0 END AS p_alloc_cpu_containers,
       CASE WHEN CAST(di.free_ssd AS NUMERIC)/di.total_ssd > 0.2 THEN (di.free_ssd / power(2,30)) / i.ssd_space_avg ELSE 0 END AS p_alloc_ssd_containers,
       ROUND(CAST((CASE WHEN CAST(di.free_cores AS NUMERIC)/di.total_cores > 0.2 THEN di.free_cores / i.cpu_avg ELSE 0 END) -
       (CASE WHEN CAST(di.free_ssd AS NUMERIC)/di.total_ssd > 0.2 THEN (di.free_ssd / power(2,30)) / i.ssd_space_avg ELSE 0 END) AS NUMERIC), 3) AS dom0_utilization_gradient_value
FROM def_allocs AS i
LEFT JOIN mdb.dom0_info AS di ON i.gen = di.generation AND i.geo = di.geo AND di.project = 'pgaas'
