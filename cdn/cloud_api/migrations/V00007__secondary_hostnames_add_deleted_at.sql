ALTER TABLE secondary_hostnames_table
    ADD COLUMN deleted_at TIMESTAMP WITH TIME ZONE NULL;

-- change unique constraint
DROP INDEX idx_secondary_hostnames_table_unique_hostname;

CREATE UNIQUE INDEX idx_secondary_hostnames_table_unique_hostname ON
    secondary_hostnames_table (hostname) WHERE resource_entity_active IS TRUE AND deleted_at IS NULL;
