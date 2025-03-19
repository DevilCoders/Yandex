ALTER TABLE dbaas.disk_type ADD COLUMN cloud_provider_disk_type text;
COMMENT ON COLUMN dbaas.disk_type.cloud_provider_disk_type IS
    'The cloud providers name of disk type (disk type that we pass to their API)';

ALTER TABLE dbaas.flavors ADD COLUMN cloud_provider_flavor_name text;
COMMENT ON COLUMN dbaas.flavors.cloud_provider_flavor_name IS
    'The cloud providers name of flavor (flavor that we allocate)';
