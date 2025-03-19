CREATE TYPE dbaas.alert_group_status AS ENUM (
	'CREATING',
    'ACTIVE',
    'DELETING',
    'CREATE-ERROR',
    'DELETE-ERROR'
);

CREATE TYPE dbaas.alert_status AS ENUM (
	'CREATING',
    'ACTIVE',
    'DELETING',
    'CREATE-ERROR',
    'DELETE-ERROR'
);

ALTER TABLE dbaas.alert_group ADD COLUMN status dbaas.alert_group_status NOT NULL;
ALTER TABLE dbaas.alert_group_revs ADD COLUMN status dbaas.alert_group_status NOT NULL;

ALTER TABLE dbaas.alert DROP CONSTRAINT pk_alert;
ALTER TABLE dbaas.alert DROP COLUMN alert_id;
ALTER TABLE dbaas.alert ADD CONSTRAINT pk_alert_group_mertic_name PRIMARY KEY (alert_group_id, metric_name);
ALTER TABLE dbaas.alert ADD COLUMN status dbaas.alert_status NOT NULL;

ALTER TABLE dbaas.alert_rev RENAME TO alert_revs;
ALTER TABLE dbaas.alert_revs ADD COLUMN status dbaas.alert_status NOT NULL;
ALTER TABLE dbaas.alert_revs DROP CONSTRAINT pk_alert_rev;
ALTER TABLE dbaas.alert_revs DROP COLUMN alert_id;
ALTER TABLE dbaas.alert_revs ADD CONSTRAINT pk_alert_group_mertic_name_rev PRIMARY KEY (alert_group_id, metric_name, rev);

ALTER TABLE dbaas.default_alert ADD COLUMN selectors TEXT NOT NULL;
