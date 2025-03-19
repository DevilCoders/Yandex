ALTER TABLE IF EXISTS cs_secrets ADD COLUMN IF NOT EXISTS secret_timestamp integer NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW());
ALTER TABLE IF EXISTS cs_secrets_history ADD COLUMN IF NOT EXISTS secret_timestamp integer;
