CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE IF NOT EXISTS "txn_test" (
    f text DEFAULT '' NOT NULL,
    s text DEFAULT '' NOT NULL,
    t text DEFAULT '' NOT NULL
);
