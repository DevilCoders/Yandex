ALTER TABLE mdb.dom0_hosts
    ADD COLUMN heartbeat timestamp with time zone;

COMMENT ON COLUMN mdb.dom0_hosts.heartbeat
    IS 'Timestamp of last hearbeat';

DROP MATERIALIZED VIEW mdb.dom0_info;

CREATE MATERIALIZED VIEW mdb.dom0_info AS
    SELECT fqdn, project, geo, allow_new_hosts, heartbeat, di.*
      FROM mdb.dom0_hosts dh, LATERAL (
        SELECT (mdb.dom0_info(fqdn)).*) di;

CREATE UNIQUE INDEX uk_dom0_info ON mdb.dom0_info USING btree (fqdn);
