ALTER TABLE mdb.dom0_hosts
    ADD COLUMN generation integer NOT NULL DEFAULT 1;

DROP VIEW mdb.dom0_info;

CREATE VIEW mdb.dom0_info AS
    SELECT fqdn, project, geo, allow_new_hosts, generation, heartbeat, di.*
      FROM mdb.dom0_hosts dh, LATERAL (
        SELECT (mdb.dom0_info(fqdn)).*) di;
