ALTER TABLE resource_table
    DROP COLUMN secondary_hostnames;

CREATE TABLE IF NOT EXISTS secondary_hostnames_table
(
    row_id                  BIGSERIAL,
    resource_entity_id      TEXT,
    resource_entity_version BIGINT,
    resource_entity_active  BOOLEAN,
    hostname                TEXT,

    CONSTRAINT pk_secondary_hostnames_table PRIMARY KEY (row_id),
    CONSTRAINT fk_secondary_hostnames_table_to_resource FOREIGN KEY
        (resource_entity_id, resource_entity_version) REFERENCES resource_table (entity_id, entity_version)
);


CREATE UNIQUE INDEX idx_secondary_hostnames_table_unique_hostname ON
    secondary_hostnames_table (hostname) WHERE resource_entity_active IS TRUE;
