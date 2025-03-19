package migrations

var createMigrationTable = &Migration{
	ID:   0,
	Name: "Creating migration table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/migrations") + ` (
		id Uint64,
		name Utf8,
		up Utf8,
		down Utf8,
		created_at Uint64,

		PRIMARY KEY (id)
	);`,
	DOWN: "DROP TABLE `license_server/migrations`;",
}
