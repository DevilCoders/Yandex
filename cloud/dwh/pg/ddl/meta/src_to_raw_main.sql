create table src_to_raw_main
(
    connection_id     varchar(255) not null
        constraint src_to_raw_main_connection_id_fk
            references connection,
    object_id         varchar(255) not null,
    source_table_name varchar(255) not null,
    target_table_name varchar(255) not null,
    write_mode        varchar(255) not null
);

create unique index src_to_raw_main_object_id_uindex
    on src_to_raw_main (object_id);

alter table src_to_raw_main
    owner to "dwh-admin";

grant select on meta.src_to_raw_main to "etl-user";
