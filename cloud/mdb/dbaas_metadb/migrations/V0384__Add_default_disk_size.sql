ALTER TABLE dbaas.valid_resources
    ADD COLUMN default_disk_size bigint,
    ADD CONSTRAINT default_disk_size_valid CHECK (
            default_disk_size IS NULL OR (
                (disk_size_range IS NULL OR disk_size_range @> default_disk_size)
                AND (disk_sizes IS NULL OR default_disk_size = ANY (disk_sizes))
            )
        );
