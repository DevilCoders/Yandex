package pgmodels

// PostgreSQLMetaDBComponent is a value from dbaas.default_versions.component
const PostgreSQLMetaDBComponent string = "postgres"

var PostgreSQLVersionsFeatureFlags = map[string]string{
	"10-1c": "MDB_POSTGRESQL_10_1C",
	"11":    "MDB_POSTGRESQL_11",
	"11-1c": "MDB_POSTGRESQL_11_1C",
	"12":    "MDB_POSTGRESQL_12",
	"12-1c": "MDB_POSTGRESQL_12_1C",
	"13":    "MDB_POSTGRESQL_13",
	"13-1c": "MDB_POSTGRESQL_13_1C",
	"14":    "MDB_POSTGRESQL_14",
	"14-1c": "MDB_POSTGRESQL_14_1C",
}

var PostgreSQLVersionedKeys = map[string]string{
	"10":    "postgresqlConfig_10",
	"10-1c": "postgresqlConfig_10_1c",
	"11":    "postgresqlConfig_11",
	"11-1c": "postgresqlConfig_11_1c",
	"12":    "postgresqlConfig_12",
	"12-1c": "postgresqlConfig_12_1c",
	"13":    "postgresqlConfig_13",
	"13-1c": "postgresqlConfig_13_1c",
	"14":    "postgresqlConfig_14",
	"14-1c": "postgresqlConfig_14_1c",
}
