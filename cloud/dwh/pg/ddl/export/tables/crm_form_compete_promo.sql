create table crm_form_compete_promo
(
    yandexuid               varchar(255),
    form_id                 bigint,
    passport_uid            bigint,
    passport_login          varchar(255),
    create_dttm             timestamp with time zone,
    company                 varchar(512),
    agreement_newsletters   varchar(10),
    phone                   varchar(255),
    email                   varchar(255),
    last_name               varchar(255),
    first_name              varchar(255),
    utm_content             varchar(1024),
    utm_source              varchar(1024),
    utm_campaign            varchar(1024),
    utm_medium              varchar(1024),
    _insert_dttm            timestamp with time zone default timezone('utc'::text, now()),
    ba_id                   varchar(255),
    timezone                varchar(255),
    lead_source             varchar(255),
    lead_source_description varchar(255),
    message                 text,
    "check"                 text,
    constraint fct_crm_form_compete_promo_pk
        unique (yandexuid, form_id)
);
