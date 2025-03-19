ALTER TABLE s3.clouds
    ADD COLUMN system_settings JSONB;

ALTER TABLE s3.accounts
    ADD COLUMN system_settings JSONB;
