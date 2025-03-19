GRANT USAGE ON SCHEMA dbaas TO mdb_dns;
GRANT USAGE ON SCHEMA code TO mdb_dns;
GRANT SELECT ON TABLE dbaas.clusters TO mdb_dns;
GRANT SELECT ON TABLE dbaas.shards TO mdb_dns;
GRANT SELECT ON TABLE dbaas.subclusters TO mdb_dns;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_dns;
