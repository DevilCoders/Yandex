create table ods.bcf_application
(
    id                bigint,
    status            varchar(255),
    created_at        timestamp with time zone,
    updated_at        timestamp with time zone,
    event_id          integer,
    participant_id    integer,
    visited           boolean,
    language          varchar(255),
    field_name        varchar(1024),
    field_value       varchar(2048),
    field_slug        varchar(255),
    _insert_dttm      timestamp with time zone default timezone('utc'::text, now()),
    _source_system_id integer                  default 2
);

grant select, update, insert, delete, truncate on ods.bcf_application to "etl-user";

