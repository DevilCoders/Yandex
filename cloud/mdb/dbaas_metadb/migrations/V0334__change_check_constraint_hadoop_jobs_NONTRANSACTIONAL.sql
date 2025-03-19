ALTER TABLE dbaas.hadoop_jobs
DROP CONSTRAINT check_hadoop_job_end_ts;
ALTER TABLE dbaas.hadoop_jobs
ADD CONSTRAINT check_hadoop_job_end_ts 
CHECK (
        status NOT IN ('ERROR', 'DONE', 'CANCELLED') OR end_ts IS NOT NULL
    );
