package migrations

var createLicenseTemplateVersionsTable = &Migration{
	ID:   2,
	Name: "Creating license template version table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/meta/license/template_versions") + ` (
		id Utf8,
		template_id Utf8,
		price Json,
		period Utf8,
		license_sku_id Utf8,
		name Utf8,

		created_at Uint64,
		updated_at Uint64,

		state  Utf8,

		PRIMARY KEY (id),
		INDEX template_versions_template_id_idx GLOBAL ON (template_id, state, id)
	);`,
	DOWN: "DROP TABLE `license_server/meta/license/template_versions`;",
}
