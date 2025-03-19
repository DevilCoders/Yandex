ALTER TABLE dbaas.default_alert ADD COLUMN template_id TEXT NULL;
UPDATE dbaas.default_alert SET template_id = '';
ALTER TABLE dbaas.default_alert ALTER COLUMN template_id SET NOT NULL;
ALTER TABLE dbaas.default_alert ADD COLUMN template_version TEXT NULL;
UPDATE dbaas.default_alert SET template_version = '';
ALTER TABLE dbaas.default_alert ALTER COLUMN template_version SET NOT NULL;
