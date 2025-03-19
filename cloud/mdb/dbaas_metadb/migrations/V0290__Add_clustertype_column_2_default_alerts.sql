ALTER TABLE dbaas.alert_group_revs DROP COLUMN cluster_type ;

ALTER TABLE dbaas.default_alert ADD COLUMN cluster_type dbaas.cluster_type NOT NULL;
ALTER TABLE dbaas.alert RENAME COLUMN alter_ext_id TO alert_ext_id;
ALTER TABLE dbaas.alert ALTER COLUMN alert_ext_id DROP NOT NULL;

ALTER TABLE dbaas.alert_rev RENAME COLUMN alter_ext_id TO alert_ext_id;
ALTER TABLE dbaas.alert_rev ALTER COLUMN alert_ext_id DROP NOT NULL;

