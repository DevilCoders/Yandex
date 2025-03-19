GRANT CONNECT ON DATABASE billingdb TO dbaas_worker;
GRANT USAGE ON SCHEMA billing TO dbaas_worker;
GRANT SELECT, INSERT, UPDATE, DELETE ON billing.tracks TO dbaas_worker;
