package migrations

var createLicenseTemplatesTable = &Migration{
	ID:   1,
	Name: "Creating license template table",
	UP: `--!syntax_v1

	CREATE TABLE ` + quote("license_server/meta/license/templates") + ` (
		id Utf8,
		template_version_id Utf8,
		publisher_id Utf8,
		product_id Utf8,
		tariff_id Utf8,
		period Utf8,
		license_sku_id Utf8,
		name Utf8,
	
		created_at Uint64,
		updated_at Uint64,
	
		state  Utf8,
	
		PRIMARY KEY (id),
		INDEX template_publisher_id_product_id_tariff_id_idx GLOBAL ON (publisher_id, product_id, tariff_id, state, id)
	);`,
	DOWN: "DROP TABLE `license_server/meta/license/templates`;",
}
