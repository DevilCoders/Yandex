DO
$$
    BEGIN
        truncate table ods.frm_application;

        insert into ods.frm_application
        (email, yandexuid, form_id, field_name, field_value, create_dttm)
        select max(email) over (partition by yandexuid, form_id) as email,
               yandexuid,
               form_id,
               field_name,
               field_value,
               create_dttm
        from (
                 select yandexuid,
                        case when field_name = 'email' then field_value end as email,
                        form_id,
                        field_name,
                        field_value,
                        create_dttm
                 from stg.frm_application
             ) as t;
    END
$$;
