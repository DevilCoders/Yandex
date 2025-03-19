GRANT CONNECT ON DATABASE billingdb TO billing_sender;
GRANT USAGE ON SCHEMA billing TO billing_sender;
GRANT SELECT, INSERT, UPDATE, DELETE ON billing.metrics_queue TO billing_sender;
