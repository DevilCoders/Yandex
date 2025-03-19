ALTER TABLE deploy.job_results
  ADD COLUMN order_id integer;

UPDATE deploy.job_results
  SET order_id = 1;

ALTER TABLE deploy.job_results ALTER COLUMN order_id SET NOT NULL;

ALTER TABLE deploy.job_results DROP CONSTRAINT pk_job_results;
ALTER TABLE deploy.job_results ADD CONSTRAINT pk_job_results PRIMARY KEY (ext_job_id, fqdn, order_id);

ALTER TABLE deploy.job_results
  ADD COLUMN job_result_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY;
