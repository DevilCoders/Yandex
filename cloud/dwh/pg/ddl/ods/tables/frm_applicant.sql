create table ods.frm_applicant
(
    email                 varchar(512) not null
        constraint frm_applicant_pk
            primary key,
    passport_uid          bigint,
    passport_login        varchar(512),
    mail_marketing        boolean,
    position              varchar(512),
    first_name            varchar(255),
    last_name             varchar(255),
    agreement_newsletters boolean,
    phone                 varchar(32),
    company               varchar(255),
    website               varchar(512)
);

grant select, update, insert, delete, truncate on ods.frm_applicant to "etl-user";
