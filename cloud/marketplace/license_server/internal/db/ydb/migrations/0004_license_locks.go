package migrations

var createLicenseLocksTable = &Migration{
	ID:   4,
	Name: "Creating license template version table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/meta/license/locks") + ` (
		id Utf8,
		instance_id Utf8,
		resource_lock_id Utf8,

		start_time Uint64,
		end_time Uint64,
		created_at Uint64,
		updated_at Uint64,

		state  Utf8,

		PRIMARY KEY (id),
		INDEX locks_instance_id_idx GLOBAL ON (instance_id, state, id)
	);`,
	DOWN: "DROP TABLE `license_server/meta/license/locks`;",
}
