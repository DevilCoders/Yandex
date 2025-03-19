package migrations

var createOperationsTable = &Migration{
	ID:   5,
	Name: "Creating operations table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/meta/operations") + ` (
		id Utf8,

		description Utf8,
		created_at Uint64,
		created_by Utf8,
		modified_at Uint64,
		done bool,
		metadata Json,
		result Json,

		PRIMARY KEY (id)
	);`,
	DOWN: "DROP TABLE `license_server/meta/operations`;",
}
