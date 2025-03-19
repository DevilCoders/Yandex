GRANT USAGE ON SCHEMA billing TO billing_bookkeeper;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA billing TO billing_bookkeeper;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA billing TO billing_bookkeeper;
GRANT CONNECT ON DATABASE billingdb TO billing_bookkeeper;
