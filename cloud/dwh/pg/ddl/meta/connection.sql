-- auto-generated definition
create table connection
(
    id              varchar(255) not null
        constraint connection_pk
            primary key,
    host            varchar(255) not null,
    port            varchar(32)  not null,
    username        varchar(255) not null,
    password_secret varchar(255) not null,
    driver_class    varchar(255) not null,
    database        varchar(255) not null
);

alter table connection
    owner to "dwh-admin";

grant select on meta.connection to "etl-user";

grant usage on schema meta to "etl-user";
