DROP TABLE dbaas.alert CASCADE;
DROP TABLE dbaas.alert_revs CASCADE;

DELETE FROM dbaas.default_alert;

ALTER TABLE dbaas.default_alert DROP CONSTRAINT pk_default_alert;

ALTER TABLE dbaas.default_alert DROP COLUMN condition;
ALTER TABLE dbaas.default_alert DROP COLUMN metric_name;
ALTER TABLE dbaas.default_alert DROP COLUMN selectors;

ALTER TABLE dbaas.default_alert ADD CONSTRAINT pk_default_alert PRIMARY KEY (template_id);

CREATE TABLE dbaas.alert (
    alert_ext_id          TEXT,
    alert_group_id        TEXT NOT NULL,
    template_id           TEXT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    default_thresholds    BOOLEAN NOT NULL,
    status                dbaas.alert_status NOT NULL,

    CONSTRAINT pk_alert_group_mertic_name PRIMARY KEY (alert_group_id, template_id),
    CONSTRAINT alert_has_thresholds CHECK (critical_threshold IS NOT NULL or warning_threshold IS NOT NULL),
    CONSTRAINT fk_alert_alert_group FOREIGN KEY (alert_group_id) REFERENCES dbaas.alert_group(alert_group_id) ON DELETE RESTRICT,
    CONSTRAINT fk_alert_default_alert FOREIGN KEY (template_id) REFERENCES dbaas.default_alert(template_id) ON DELETE RESTRICT
);

CREATE TABLE dbaas.alert_revs (
    alert_ext_id          TEXT,
    alert_group_id        TEXT NOT NULL,
    template_id           TEXT NOT NULL,
    rev                   BIGINT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    default_thresholds    BOOLEAN NOT NULL,
    status                dbaas.alert_status NOT NULL,

    CONSTRAINT pk_alert_group_mertic_name_rev PRIMARY KEY (alert_group_id, template_id, rev),
    CONSTRAINT fk_alert_alert_group_rev FOREIGN KEY (alert_group_id, rev) REFERENCES dbaas.alert_group_revs(alert_group_id, rev) ON DELETE RESTRICT
);

ALTER TABLE dbaas.alert_group DROP COLUMN notification_channels;
ALTER TABLE dbaas.alert_group DROP COLUMN disabled;

ALTER TABLE dbaas.alert_group_revs DROP COLUMN notification_channels;
ALTER TABLE dbaas.alert_group_revs DROP COLUMN disabled;

