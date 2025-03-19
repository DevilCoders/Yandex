create table ods.iam_cloud_creator
(
    timezone               varchar(255),
    mail_tech              boolean,
    mail_feature           boolean,
    mail_support           boolean,
    mail_billing           boolean,
    mail_promo             boolean,
    mail_testing           boolean,
    mail_marketing         boolean,
    mail_info              boolean,
    mail_event             boolean,
    mail_technical         boolean,
    cloud_id               varchar(255),
    user_settings_email    varchar(512),
    cloud_status           varchar(512),
    login                  varchar(512),
    passport_uid           bigint,
    first_name             varchar(255),
    last_name              varchar(256),
    email                  varchar(255),
    id                     varchar(255),
    user_settings_language varchar(255),
    cloud_name             varchar(255),
    phone                  varchar(255),
    cloud_created_at       timestamp with time zone,
    _insert_dttm           timestamp default timezone('utc'::text, now())
);
grant select, update, insert, delete, truncate on ods.iam_cloud_creator to "etl-user";
