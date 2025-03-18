ALTER TABLE resource_table DROP COLUMN edge_cache_default_ttl;
ALTER TABLE resource_table DROP COLUMN edge_cache_override_ttl;

ALTER TABLE resource_table ADD COLUMN edge_cache_ttl BIGINT;
ALTER TABLE resource_table ADD COLUMN edge_cache_override BOOLEAN;
