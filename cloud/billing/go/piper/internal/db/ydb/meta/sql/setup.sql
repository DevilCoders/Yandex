CREATE TABLE `test/meta/schemas` (
    service_id Utf8,
    name Utf8,
    tags Json,
    PRIMARY KEY (name)
);

CREATE TABLE `test/meta/units` (
    src_unit Utf8,
    dst_unit Utf8,
    factor   Uint64,
    reverse  Bool,
    type     Utf8,
    PRIMARY KEY (src_unit, dst_unit)
);

CREATE TABLE `test/utility/conversion_rates` (
    source_currency Utf8,
    target_currency Utf8,
    effective_time Uint64,
    multiplier Decimal(22,9),
    PRIMARY KEY (source_currency, target_currency, effective_time)
);

CREATE TABLE `test/meta/skus` (
    metric_unit Utf8,
    rate_formula Utf8,
    pricing_versions Json,
    resolving_policy Utf8,
    pricing_unit Utf8,
    usage_unit Utf8,
    id Utf8,
    priority Uint64,
    balance_product_id Utf8,
    formula Utf8,
    service_id Utf8,
    name Utf8,
    created_at Uint64,
    publisher_account_id Utf8,
    translations Json,
    updated_at Uint64,
    updated_by Utf8,
    usage_type Utf8,
    resolving_rules Json,
    deprecated Bool,
    PRIMARY KEY (id)
);

CREATE TABLE `test/meta/skus_name_id_idx` (
    id Utf8,
    name Utf8,
    PRIMARY KEY (name, id)
);

CREATE TABLE `test/meta/schema_to_skus` (
    schema Utf8,
    sku_id Utf8,
    PRIMARY KEY (schema, sku_id)
);

CREATE TABLE `test/meta/skus_usage_type_id_idx` (
    usage_type Utf8,
    id Utf8,
    PRIMARY KEY (usage_type, id)
);

CREATE TABLE `test/meta/product_to_skus_v2` (
    product_id Utf8,
    sku_id Utf8,
    hash Uint64,
    check_formula Utf8,
    resolving_rules Json,
    PRIMARY KEY (product_id, sku_id, hash)
);

CREATE TABLE `test/meta/product_to_skus_v2_sku_id_idx` (
    sku_id Utf8,
    product_id Utf8,
    hash Uint64,
    PRIMARY KEY (sku_id, product_id, hash)
);

CREATE TABLE `test/meta/schema_to_skus_sku_id_idx` (
    schema Utf8,
    sku_id Utf8,
    PRIMARY KEY (sku_id, schema)
);

CREATE TABLE `test/meta/skus_service_id_id_idx` (
    id Utf8,
    service_id Utf8,
    PRIMARY KEY (service_id, id)
);

CREATE TABLE `test/meta/billing_accounts` (
    updated_at Uint64,
    balance Decimal(22,9),
    balance_client_id Utf8,
    id Utf8,
    payment_method_id Utf8,
    currency Utf8,
    person_id Utf8,
    payment_cycle_type Utf8,
    created_at Uint64,
    state Utf8,
    person_type Utf8,
    metadata Json,
    master_account_id Utf8,
    type Utf8,
    payment_type Utf8,
    feature_flags Json,
    owner_id Utf8,
    balance_contract_id Utf8,
    usage_status Utf8,
    client_id Utf8,
    billing_threshold Decimal(22,9),
    name Utf8,
    country_code Utf8,
    paid_at Uint64,
    disabled_at Uint64,
    PRIMARY KEY (id)
);

CREATE TABLE `test/meta/billing_accounts_history` (
    updated_at Uint64,
    balance Decimal(22,9),
    balance_client_id Utf8,
    billing_account_id Utf8,
    payment_method_id Utf8,
    currency Utf8,
    person_id Utf8,
    payment_cycle_type Utf8,
    created_at Uint64,
    state Utf8,
    person_type Utf8,
    metadata Json,
    master_account_id Utf8,
    type Utf8,
    payment_type Utf8,
    feature_flags Json,
    owner_id Utf8,
    balance_contract_id Utf8,
    usage_status Utf8,
    client_id Utf8,
    billing_threshold Decimal(22,9),
    name Utf8,
    country_code Utf8,
    paid_at Uint64,
    disabled_at Uint64,
    PRIMARY KEY (billing_account_id, updated_at)
);

CREATE TABLE `test/meta/billing_accounts_master_account_id_idx` (
    id Utf8,
    master_account_id Utf8,
    PRIMARY KEY (master_account_id, id)
);

CREATE TABLE `test/meta/billing_accounts_owner_id_idx` (
    owner_id Utf8,
    id Utf8,
    PRIMARY KEY (owner_id, id)
);

CREATE TABLE `test/meta/billing_accounts_balance_client_id_idx` (
    id Utf8,
    balance_client_id Utf8,
    PRIMARY KEY (balance_client_id, id)
);

CREATE TABLE `test/meta/billing_accounts_client_id_idx` (
    id Utf8,
    client_id Utf8,
    PRIMARY KEY (client_id, id)
);

CREATE TABLE `test/meta/billing_accounts_balance_contract_id_idx` (
    balance_contract_id Utf8,
    id Utf8,
    PRIMARY KEY (balance_contract_id, id)
);

CREATE TABLE `test/meta/billing_accounts_state_idx` (
    id Utf8,
    state Utf8,
    PRIMARY KEY (state, id)
);

CREATE TABLE `test/meta/service_instance_bindings/bindings` (
    service_instance_type Utf8,
    service_instance_id Utf8,
    billing_account_id Utf8,
    effective_time Uint64,
    created_at Datetime,
    PRIMARY KEY (service_instance_type, service_instance_id, effective_time)
);

CREATE TABLE `test/meta/service_instance_bindings/bindings_billing_account_id_idx` (
    service_instance_type Utf8,
    service_instance_id Utf8,
    billing_account_id Utf8,
    effective_time Uint64,
    PRIMARY KEY (billing_account_id, service_instance_type, service_instance_id, effective_time)
);
