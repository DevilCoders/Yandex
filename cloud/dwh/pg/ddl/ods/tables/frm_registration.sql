create table ods.frm_registration
(
    yandexuid      varchar(255),
    form_id        bigint,
    passport_uid   bigint,
    passport_login varchar(255),
    field_name     varchar(255),
    field_value    text,
    create_dttm    timestamp with time zone,
    _insert_dttm   timestamp with time zone default timezone('utc'::text, now()),
    constraint frm_registration_pk
        unique (yandexuid, form_id, field_name)
);

grant select, update, insert, delete, truncate on ods.frm_registration to "etl-user";
