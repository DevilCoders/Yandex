create table ods.crm_account
(
    id                         varchar(255),
    name                       varchar(255),
    date_entered               timestamp with time zone,
    date_modified              timestamp with time zone,
    description                text,
    deleted                    boolean,
    account_type               varchar(255),
    industry                   varchar(255),
    annual_revenue             double precision,
    billing_address_street     varchar(1024),
    billing_address_city       varchar(255),
    billing_address_state      varchar(255),
    billing_address_postalcode varchar(255),
    billing_address_country    varchar(255),
    rating                     varchar(255),
    phone_office               varchar(255),
    phone_alternate            varchar(255),
    full_name                  varchar(512),
    last_name                  varchar(128),
    first_name                 varchar(128),
    org_type                   varchar(255),
    segment                    varchar(255),
    cloud_budget               double precision,
    timezone                   varchar(128),
    isv                        boolean,
    var                        boolean,
    person_type                varchar(255),
    _insert_dttm               timestamp with time zone default timezone('utc'::text, now()),
    _source_system_id          integer                  default 3
);

grant select, update, insert, delete, truncate on ods.crm_account to "etl-user";
