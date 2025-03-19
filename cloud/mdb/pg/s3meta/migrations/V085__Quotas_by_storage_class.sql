ALTER TABLE s3.clouds ADD COLUMN additional_quotas JSONB ;
ALTER TABLE s3.accounts ADD COLUMN additional_quotas JSONB;
ALTER TABLE s3.buckets ADD COLUMN additional_quotas JSONB;
