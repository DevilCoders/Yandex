GRANT USAGE ON SCHEMA dbaas TO mdb_downtimer;

GRANT SELECT ON TABLE dbaas.clusters TO mdb_downtimer;
GRANT SELECT ON TABLE dbaas.folders TO mdb_downtimer;
GRANT SELECT ON TABLE dbaas.subclusters TO mdb_downtimer;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO mdb_downtimer;

GRANT USAGE ON SCHEMA code TO mdb_downtimer;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO mdb_downtimer;
