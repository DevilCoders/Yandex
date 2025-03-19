create table ods.bcf_applicant
(
    participant_id    integer not null
        constraint bcf_applicant_pk
            primary key,
    first_name        varchar(255),
    last_name         varchar(255),
    company           varchar(512),
    position          varchar(512),
    phone             varchar(128),
    email             varchar(255),
    website           varchar(512),
    has_cloud_account boolean
);

grant select, update, insert, delete, truncate on ods.bcf_applicant to "etl-user";

