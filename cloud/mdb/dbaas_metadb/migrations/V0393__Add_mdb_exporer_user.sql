GRANT USAGE ON SCHEMA dbaas TO mdb_exporter;
GRANT SELECT ON TABLE dbaas.clusters TO mdb_exporter;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_exporter;
GRANT USAGE ON SCHEMA code TO mdb_exporter;

GRANT EXECUTE ON FUNCTION code.managed(dbaas.clusters) TO mdb_exporter;
GRANT EXECUTE ON FUNCTION code.visible(dbaas.clusters) TO mdb_exporter;
GRANT EXECUTE ON FUNCTION dbaas.error_cluster_status(dbaas.cluster_status) TO mdb_exporter;

