CREATE TYPE dbaas.alert_condition AS ENUM (
    'LESS',
    'LESS_OR_EQUAL',
    'EQUAL',
    'NOT_EQUAL',
    'GREATER_OR_EQUAL',
    'GREATER'
);

CREATE TABLE dbaas.alert_group(
    alert_group_id        TEXT NOT NULL,
    cid                   TEXT NOT NULL,
    monitoring_folder_id  TEXT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    managed               BOOLEAN NOT NULL DEFAULT FALSE,

    CONSTRAINT pk_alert_group PRIMARY KEY (alert_group_id),
    CONSTRAINT fk_alert_group_cid FOREIGN KEY (cid) REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_alert_group_cid_id ON dbaas.alert_group (cid, alert_group_id);

CREATE TABLE dbaas.alert_group_revs(
    alert_group_id        TEXT NOT NULL,
    rev                   BIGINT NOT NULL,
    cid                   TEXT NOT NULL,
    cluster_type          dbaas.cluster_type NOT NULL,
    monitoring_folder_id  TEXT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    managed               BOOLEAN NOT NULL DEFAULT FALSE,

    CONSTRAINT pk_alert_group_revs PRIMARY KEY (alert_group_id, rev),
    CONSTRAINT fk_alert_group_revs_cid FOREIGN KEY (cid, rev) REFERENCES dbaas.clusters_revs(cid, rev) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_alert_group_revs_cid_id ON dbaas.alert_group_revs (cid, rev, alert_group_id);

CREATE TABLE dbaas.alert (
    alert_id              BIGSERIAL NOT NULL,
    alter_ext_id          TEXT NOT NULL,
    alert_group_id        TEXT NOT NULL,
    metric_name           TEXT NOT NULL,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    condition             dbaas.alert_condition NOT NULL,
    default_thresholds    BOOLEAN NOT NULL,

    CONSTRAINT pk_alert PRIMARY KEY (alert_id),
    CONSTRAINT fk_alert_alert_group FOREIGN KEY (alert_group_id) REFERENCES dbaas.alert_group(alert_group_id) ON DELETE RESTRICT
);

CREATE TABLE dbaas.alert_rev (
    alert_id              BIGSERIAL NOT NULL,
    alter_ext_id          TEXT NOT NULL,
    alert_group_id        TEXT NOT NULL,
    rev                   BIGINT NOT NULL,
    metric_name           TEXT NOT NULL,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    condition             dbaas.alert_condition NOT NULL,
    default_thresholds    BOOLEAN NOT NULL,

    CONSTRAINT pk_alert_rev PRIMARY KEY (alert_id, rev),
    CONSTRAINT fk_alert_alert_group_rev FOREIGN KEY (alert_group_id, rev) REFERENCES dbaas.alert_group_revs(alert_group_id, rev) ON DELETE RESTRICT
);

CREATE TABLE dbaas.default_alert(
    metric_name        TEXT NOT NULL,
    critical_threshold NUMERIC,
    warning_threshold  NUMERIC,
    condition          dbaas.alert_condition NOT NULL,
    
    CONSTRAINT pk_default_alert PRIMARY KEY (metric_name)
);
