CREATE INDEX CONCURRENTLY i_pillar_expiration_fqdn
    ON dbaas.pillar ((dbaas.string_to_iso_timestamptz(value ->> 'cert.expiration')), fqdn)
    WHERE fqdn IS NOT NULL;
