package migrations

var createLicenseInstancesTable = &Migration{
	ID:   3,
	Name: "Creating license template version table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/meta/license/instances") + ` (
		id Utf8,
		template_id Utf8,
		template_version_id Utf8,
		cloud_id Utf8,
		name Utf8,

		start_time Uint64,
		end_time Uint64,
		created_at Uint64,
		updated_at Uint64,

		state  Utf8,

		PRIMARY KEY (id),
		INDEX instances_cloud_id_idx GLOBAL ON (cloud_id, state, id),
		INDEX instances_state_end_time_idx GLOBAL ON (state, end_time, id),
		INDEX instances_state_start_time_idx GLOBAL ON (state, start_time, id)
	);`,
	DOWN: "DROP TABLE `license_server/meta/license/instances`;",
}
