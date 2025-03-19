CREATE INDEX i_dom0_hosts_geo_project_heartbeat ON mdb.dom0_hosts (geo, project, heartbeat);
CREATE INDEX i_containers_cluster_name ON mdb.containers (cluster_name);

DROP MATERIALIZED VIEW mdb.dom0_info;

CREATE VIEW mdb.dom0_info AS
    SELECT fqdn, project, geo, allow_new_hosts, heartbeat, di.*
      FROM mdb.dom0_hosts dh, LATERAL (
        SELECT (mdb.dom0_info(fqdn)).*) di;
