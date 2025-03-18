-- secondary hostnames

BEGIN;

ALTER TABLE secondary_hostnames_table
    DROP CONSTRAINT fk_secondary_hostnames_table_to_resource;

ALTER TABLE secondary_hostnames_table
    ADD CONSTRAINT fk_secondary_hostnames_table_to_resource FOREIGN KEY
        (resource_entity_id, resource_entity_version) REFERENCES resource_table (entity_id, entity_version) ON DELETE CASCADE;

COMMIT;

-- origins

BEGIN;

ALTER TABLE origin_table
    DROP CONSTRAINT fk_origin_table_to_origins_group_table;

ALTER TABLE origin_table
    ADD CONSTRAINT fk_origin_table_to_origins_group_table FOREIGN KEY
        (origins_group_entity_id, origins_group_entity_version) REFERENCES origins_group_table (entity_id, entity_version) ON DELETE CASCADE;

COMMIT;

-- resource rule

BEGIN;

ALTER TABLE resource_rule_table
    DROP CONSTRAINT fk_resource_rule_table_to_resource_table;

ALTER TABLE resource_rule_table
    ADD CONSTRAINT fk_resource_rule_table_to_resource_table FOREIGN KEY
        (resource_entity_id, resource_entity_version) REFERENCES resource_table (entity_id, entity_version) ON DELETE CASCADE ;

COMMIT;
