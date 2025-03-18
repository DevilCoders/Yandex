CREATE TABLE IF NOT EXISTS origins_group_table
(
    row_id         BIGSERIAL,
    entity_id      BIGINT                   NOT NULL,
    entity_version BIGINT                   NOT NULL,
    entity_active  BOOLEAN                  NOT NULL,

    folder_id      TEXT                     NOT NULL,
    name           TEXT,
    use_next       BOOLEAN                  NOT NULL,

    created_at     TIMESTAMP WITH TIME ZONE NOT NULL,
    updated_at     TIMESTAMP WITH TIME ZONE NOT NULL,
    deleted_at     TIMESTAMP WITH TIME ZONE NULL,

    CONSTRAINT pk_origins_group_table PRIMARY KEY (row_id),
    CONSTRAINT uk_origins_group_table_entity UNIQUE (entity_id, entity_version)
);

CREATE UNIQUE INDEX idx_origins_group_table_unique_entity_key ON
    origins_group_table (entity_id, entity_active) WHERE entity_active IS NOT FALSE;

CREATE TABLE IF NOT EXISTS origin_table
(
    entity_id                    BIGSERIAL,
    origins_group_entity_id      BIGINT                   NOT NULL,
    origins_group_entity_version BIGINT                   NOT NULL,

    folder_id                    TEXT                     NOT NULL,
    source                       TEXT                     NOT NULL,
    enabled                      BOOLEAN                  NOT NULL,
    backup                       BOOLEAN                  NOT NULL,
    type                         TEXT                     NOT NULL,

    created_at                   TIMESTAMP WITH TIME ZONE NOT NULL,
    updated_at                   TIMESTAMP WITH TIME ZONE NOT NULL,
    deleted_at                   TIMESTAMP WITH TIME ZONE NULL,

    CONSTRAINT pk_origin_table PRIMARY KEY (entity_id),
    CONSTRAINT fk_origin_table_to_origins_group_table FOREIGN KEY
        (origins_group_entity_id, origins_group_entity_version) REFERENCES origins_group_table (entity_id, entity_version)
);

CREATE TABLE IF NOT EXISTS resource_table
(
    row_id                           BIGSERIAL,
    entity_id                        TEXT                     NOT NULL,
    entity_version                   BIGINT                   NOT NULL,
    entity_active                    BOOLEAN                  NOT NULL,

    origins_group_entity_id          BIGINT                   NOT NULL,
    folder_id                        TEXT                     NOT NULL,

    active                           BOOLEAN                  NOT NULL,
    name                             TEXT,
    cname                            TEXT,
    secondary_hostnames              JSONB,

    origin_protocol                  TEXT,
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
    edge_cache_default_ttl           BIGINT,
    edge_cache_override_ttl          BIGINT,
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

    CONSTRAINT pk_resource_table PRIMARY KEY (row_id)
);

CREATE UNIQUE INDEX idx_resource_table_unique_entity_key ON
    resource_table (entity_id, entity_active) WHERE entity_active IS NOT FALSE;
