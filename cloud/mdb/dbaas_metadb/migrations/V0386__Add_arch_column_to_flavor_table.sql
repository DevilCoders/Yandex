ALTER TABLE dbaas.flavors ADD COLUMN arch text DEFAULT 'amd64' NOT NULL;
COMMENT ON COLUMN dbaas.flavors.arch IS
    'The architecture of flavor (flavor that we allocate)';
