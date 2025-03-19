SELECT
	major_version,
	minor_version,
	name,
	edition,
	is_default,
	is_deprecated,
	updatable_to
FROM	dbaas.default_versions
WHERE
	type = %(type)s AND
	component = %(component)s AND
	env = %(env)s
