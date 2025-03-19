CREATE OR REPLACE FUNCTION dbaas.visible_alert_status(
    dbaas.alert_status
) RETURNS bool AS $$
SELECT $1 IN (
	'ACTIVE',
	'CREATING',
	'UPDATING',
	'CREATE-ERROR'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION dbaas.visible_alert_group_status(
    dbaas.alert_group_status
) RETURNS bool AS $$
SELECT $1 IN (
	'ACTIVE',
	'CREATING',
	'CREATE-ERROR'
);
$$ LANGUAGE SQL IMMUTABLE;

