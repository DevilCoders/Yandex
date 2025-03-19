CREATE TABLE secrets.certs
(
  host        text        NOT NULL PRIMARY KEY,
  ca          text        NOT NULL,
  key         jsonb       NOT NULL,
  crt         text        NOT NULL,
  expiration  timestamptz NOT NULL,
  alt_names   text[]      NOT NULL DEFAULT array[]::text[],
  created_at  timestamptz NOT NULL DEFAULT now(),
  modified_at timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX i_certs_expiration ON secrets.certs (expiration);

CREATE INDEX i_certs_modified_at ON secrets.certs(modified_at);

CREATE INDEX i_certs_ca ON secrets.certs(ca);
