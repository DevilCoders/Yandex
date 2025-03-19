SELECT geo,
       generation,
       sum(free_cores)  as free_cores,
       sum(total_cores) as total_cores
FROM mdb.dom0_info
WHERE (project = 'pgaas' and heartbeat > now()::date - 1)
GROUP BY generation, geo;
