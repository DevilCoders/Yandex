ALTER TABLE dbaas.maintenance_tasks ADD COLUMN max_delay timestamp with time zone;
UPDATE dbaas.maintenance_tasks SET max_delay = plan_ts + INTERVAL '21 0:00:00';
ALTER TABLE dbaas.maintenance_tasks ALTER COLUMN max_delay SET NOT NULL ;
