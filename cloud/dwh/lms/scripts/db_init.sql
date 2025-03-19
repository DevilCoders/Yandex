create table if not exists lms_ctrl_file_load_list
(
    object_id          varchar(255)                                       not null,
    file               varchar(4000)                                      not null,
    load_id            integer                                            not null,
    is_for_load        boolean                                            not null,
    insert_dttm        timestamp with time zone default CURRENT_TIMESTAMP not null,
    file_modified_dttm timestamp                                          not null,
    is_loaded          boolean                                            not null,
    unique(object_id, load_id, file)
);

create index if not exists lms_ctrl_file_load_list_object_id_file_insert_dttm_index
    on lms_ctrl_file_load_list (object_id, file, insert_dttm);

create index if not exists lms_ctrl_file_load_list_load_id_index
    on lms_ctrl_file_load_list (load_id);

create table if not exists lms_ctrl_increment_value
(
    object_id             varchar(255) not null unique,
    increment_value       varchar(255) not null
);

grant select, update, insert, delete on lms_ctrl_file_load_list to lms_user;
grant select, update, insert, delete on lms_ctrl_increment_value to lms_user;