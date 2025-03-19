create table crm_ba_request
(
    yandexuid      varchar(255),
    form_id        bigint,
    passport_uid   bigint,
    passport_login varchar(255),
    create_dttm    timestamp with time zone,
    ba_id          varchar(255),
    crm_lead_id    varchar(255),
    _insert_dttm   timestamp with time zone default timezone('utc'::text, now()),
    constraint crm_ba_request_pk
        unique (yandexuid, form_id)
);
