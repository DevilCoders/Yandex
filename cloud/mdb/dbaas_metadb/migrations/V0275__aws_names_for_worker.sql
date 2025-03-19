UPDATE dbaas.geo SET name = 'eu-central-1a' WHERE name = 'aws-frankfurt';
UPDATE dbaas.disk_type SET disk_type_ext_id = 'gp2' WHERE disk_type_ext_id = 'aws-gp2-ssd';
UPDATE dbaas.flavors SET name = 't2.nano' WHERE name = 's1.nano' AND type = 'aws-standard';
