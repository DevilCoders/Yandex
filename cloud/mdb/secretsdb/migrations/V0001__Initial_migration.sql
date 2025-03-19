CREATE SCHEMA secrets;

CREATE TABLE secrets.gpg_keys
(
  cid        text        NOT NULL PRIMARY KEY,
  gpg_key    jsonb       NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now()
);
