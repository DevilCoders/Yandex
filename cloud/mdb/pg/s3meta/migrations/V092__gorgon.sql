CREATE TABLE IF NOT EXISTS s3.https_configs
(
    bucket_name           TEXT PRIMARY KEY,
    cm_certificate_id     TEXT,
    certificates_chain    TEXT,
    encrypted_private_key TEXT,
    nonce                 TEXT,
    issuer                TEXT,
    subject               TEXT,
    dns_names             TEXT,
    not_before_ts_us      BIGINT, -- unix timestamp, microseconds
    not_after_ts_us       BIGINT -- unix timestamp, microseconds
);
