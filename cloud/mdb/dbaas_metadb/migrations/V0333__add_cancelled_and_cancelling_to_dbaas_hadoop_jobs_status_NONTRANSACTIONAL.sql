ALTER TYPE dbaas.hadoop_jobs_status ADD VALUE 'CANCELLED';
ALTER TYPE dbaas.hadoop_jobs_status ADD VALUE 'CANCELLING' AFTER 'PENDING';
