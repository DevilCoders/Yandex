CREATE INDEX CONCURRENTLY i_job_results_fqdn ON deploy.job_results USING HASH(fqdn);
CREATE INDEX CONCURRENTLY i_minions_deleted_updated_at_minion_id_fqdn ON deploy.minions (updated_at, minion_id, fqdn) WHERE deleted;
