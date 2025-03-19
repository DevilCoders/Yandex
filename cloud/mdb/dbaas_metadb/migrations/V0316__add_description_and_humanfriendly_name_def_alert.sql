ALTER TABLE dbaas.default_alert ADD COLUMN description TEXT;

ALTER TABLE dbaas.default_alert ADD COLUMN name TEXT;

ALTER TABLE dbaas.default_alert ADD COLUMN mandatory BOOLEAN DEFAULT FALSE;
