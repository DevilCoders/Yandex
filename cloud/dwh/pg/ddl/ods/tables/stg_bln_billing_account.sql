create table ods.stg_bln_billing_account
(
    name                varchar(255),
    balance             varchar(255),
    payment_method_id   varchar(255),
    export_ts           timestamp with time zone,
    person_id           varchar(255),
    currency            varchar(8),
    state               varchar(255),
    balance_client_id   varchar(255),
    person_type         varchar(255),
    ba_id               varchar(255),
    country_code        varchar(8),
    payment_cycle_type  varchar(32),
    owner_id            bigint,
    master_account_id   varchar(255),
    usage_status        varchar(128),
    type                varchar(64),
    billing_threshold   varchar(255),
    created_at          timestamp with time zone,
    balance_contract_id varchar(255),
    updated_at          timestamp with time zone,
    client_id           varchar(255),
    payment_type        varchar(64),
    is_isv              boolean,
    is_var              boolean,
    block_reason        varchar(512),
    unblock_reason      varchar(512),
    paid_at             timestamp with time zone,
    _insert_dttm        timestamp with time zone default timezone('utc'::text, now()),
    _source_system_id   integer                  default 1
);

grant select, update, insert, delete, truncate on ods.stg_bln_billing_account to "etl-user";
