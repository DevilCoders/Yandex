ALTER TABLE resource_table
    ADD CONSTRAINT uk_resource_table_entity UNIQUE (entity_id, entity_version);
