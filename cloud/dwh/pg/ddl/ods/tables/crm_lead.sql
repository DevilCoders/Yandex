create table ods.crm_lead
(
    id                varchar(255),
    date_entered      timestamp with time zone,
    date_modified     timestamp with time zone,
    description       text,
    deleted           boolean,
    first_name        varchar(128),
    last_name         varchar(128),
    title             varchar(255),
    department        varchar(255),
    phone_home        varchar(64),
    phone_mobile      varchar(64),
    phone_work        varchar(64),
    phone_other       varchar(64),
    converted         boolean,
    lead_source       varchar(64),
    status            varchar(64),
    account_id        varchar(255),
    website           varchar(255),
    passport_uid      double precision,
    do_not_call       boolean,
    _insert_dttm      timestamp with time zone default timezone('utc'::text, now()),
    _source_system_id integer                  default 3
);

grant select, update, insert, delete, truncate on ods.crm_lead to "etl-user";
