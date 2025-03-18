CREATE TABLE IF NOT EXISTS resource_rule_table
(
    entity_id                        BIGSERIAL,
    resource_entity_id               TEXT   NOT NULL,
    resource_entity_version          BIGINT NOT NULL,

    name                             TEXT   NOT NULL,
    pattern                          TEXT   NOT NULL,

    origins_group_entity_id          BIGINT,
    origin_protocol                  TEXT,

    -- options
    custom_host                      TEXT,
    custom_sni                       TEXT,
    redirect_to_https                BOOLEAN,
    allowed_methods                  JSONB,

    cors_enabled                     BOOLEAN,
    cors_enable_timing               BOOLEAN,
    cors_mode                        TEXT,
    cors_allowed_origins             JSONB,
    cors_allowed_methods             JSONB,
    cors_allowed_headers             JSONB,
    cors_max_age                     BIGINT,
    cors_expose_headers              JSONB,

    browser_cache_enabled            BOOLEAN,
    browser_cache_max_age            BIGINT,

    edge_cache_enabled               BOOLEAN,
    edge_cache_use_redirects         BOOLEAN,
    edge_cache_ttl                   BIGINT,
    edge_cache_override              BOOLEAN,
    edge_cache_override_ttl_codes    JSONB,

    serve_stale_enabled              BOOLEAN,
    serve_stale_errors               JSONB,

    normalize_request_cookies_ignore BOOLEAN,
    normalize_request_query_string   JSONB,

    compression_variant              JSONB,

    static_headers_request           JSONB,
    static_headers_response          JSONB,

    rewrite_enabled                  BOOLEAN,
    rewrite_regex                    TEXT,
    rewrite_replacement              TEXT,
    rewrite_flag                     TEXT,

    created_at                       TIMESTAMP WITH TIME ZONE NOT NULL,
    updated_at                       TIMESTAMP WITH TIME ZONE NOT NULL,
    deleted_at                       TIMESTAMP WITH TIME ZONE NULL,

    CONSTRAINT pk_resource_rule_table PRIMARY KEY (entity_id),
    CONSTRAINT fk_resource_rule_table_to_resource_table FOREIGN KEY
        (resource_entity_id, resource_entity_version) REFERENCES resource_table (entity_id, entity_version)
);
