GRANT USAGE ON SCHEMA dbaas TO idm_service;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO idm_service;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO idm_service;
GRANT USAGE ON SCHEMA code TO idm_service;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO idm_service;
