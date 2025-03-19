CREATE INDEX CONCURRENTLY i_jobs_status ON deploy.jobs (status);

ALTER TYPE deploy.job_result_status ADD VALUE 'NOTRUNNING';
