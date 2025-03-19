ALTER TABLE mdb.dom0_hosts
    ADD COLUMN allow_new_hosts_updated_by text;

COMMENT ON COLUMN mdb.dom0_hosts.allow_new_hosts_updated_by
    IS 'Last user to update allow_new_hosts';
