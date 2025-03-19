ALTER TABLE dbaas.pillar ADD CONSTRAINT check_host_cert_expiration_valid_timestamp_with_tz CHECK (
     value->>'cert.expiration'
          ~ '^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{1,6})?(Z|[+-][0-9]{2}:[0-9]{2})$'
) NOT VALID;

COMMENT ON CONSTRAINT check_host_cert_expiration_valid_timestamp_with_tz ON dbaas.pillar
    IS 'check that cert.expiration contains timestamp with specified time zone';

CREATE OR REPLACE FUNCTION dbaas.string_to_iso_timestamptz(ts text) RETURNS timestamptz
    LANGUAGE SQL
    IMMUTABLE STRICT AS
$function$
SELECT ts::timestamptz;
$function$;
