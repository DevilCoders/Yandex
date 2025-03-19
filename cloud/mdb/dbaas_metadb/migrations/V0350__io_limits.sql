ALTER TABLE dbaas.disk_type ADD COLUMN allocation_unit_size bigint DEFAULT NULL;
ALTER TABLE dbaas.disk_type ADD COLUMN io_limit_per_allocation_unit bigint DEFAULT NULL;
ALTER TABLE dbaas.disk_type ADD COLUMN io_limit_max bigint DEFAULT NULL;

ALTER TABLE dbaas.disk_type ADD CONSTRAINT check_limit_max CHECK (num_nulls(allocation_unit_size, io_limit_per_allocation_unit, io_limit_max) IN (0, 3));
COMMENT ON CONSTRAINT check_limit_max ON dbaas.disk_type IS 'All disk io_limit components should be defined or not defined at all';
