create table export.crm_lead_event_attendance
(
    first_name              varchar(255),
    last_name               varchar(255),
    email                   varchar(255),
    phone                   varchar(32),
    company                 varchar(512),
    position                varchar(512),
    ba_id                   varchar(255),
    mail_marketing          smallint,
    needs_call              smallint,
    registration_status     varchar(255),
    visited                 smallint,
    event_date              timestamp with time zone,
    event_name              varchar(255),
    event_link              varchar(255),
    event_form_id           varchar(255),
    event_city              varchar(255),
    event_is_online         smallint,
    lead_source             text,
    created_at              timestamp with time zone,
    lead_source_description text,
    id                      uuid default uuid_generate_v4(),
    lead_id                 integer not null,
    event_id                integer not null,
    constraint crm_lead_event_attendance_pk
        primary key (lead_id, event_id)
);
