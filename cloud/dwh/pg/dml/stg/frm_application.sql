do
$$
    begin
        truncate table stg.stg_frm_application;
        truncate table stg.tmp_frm_application;

        insert into stg.stg_frm_application
        (yandexuid, form_id, passport_uid, passport_login, field_name, field_value, create_dttm)
        with empty_yandexuid_map as (
            select reg2.id,
                   'fake_yandexuid_' || md5(string_agg(trim(lower(cast(each_outer.value as text))), '')) as yandexuid
            from raw.frm_registration reg2,
                 lateral jsonb_each(reg2.data) each_outer(key, value)
            where reg2.headers -> 'X-Cookies' ->> 'yandexuid' is null
            group by reg2.id
        )
        select coalesce(raw.yandexuid, fake_uid.yandexuid) as yandexuid,
               form_id,
               passport_uid,
               passport_login,
               field_name,
               field_value,
               create_dttm
        from (
                 select t4.yandexuid,
                        t4.form_id,
                        case
                            when t4.passport_uid = ''::text then null::bigint
                            else t4.passport_uid::bigint
                            end as passport_uid,
                        case
                            when t4.passport_login = ''::text then null::text
                            else t4.passport_login
                            end as passport_login,
                        t4.field_name,
                        t4.field_value,
                        t4.create_dttm,
                        t4.id
                 from (select t3.id                                                                   as id,
                              ((t3.headers -> 'X-Cookies'::text) ->> 'yandexuid'::text)::varchar(255) as yandexuid,
                              (t3.headers ->> 'X-Form-Id'::text)::bigint                              as form_id,
                              t3.headers ->> 'X-Passport-Uid'::text                                   as passport_uid,
                              t3.headers ->> 'X-Passport-Login'::text                                 as passport_login,
                              max(t3.field_name)                                                      as field_name,
                              max(t3.field_value::character varying::text)                            as field_value,
                              max(t3.create_dttm)                                                     as create_dttm
                       from (select t2.data,
                                    t2.headers,
                                    t2.field_id,
                                    t2.create_dttm,
                                    t2.id,
                                    case
                                        when each_inner.key = 'question'::text then each_inner.value ->> 'slug'::text
                                        else null::text
                                        end as field_name,
                                    case
                                        when each_inner.key = 'value'::text then each_inner.value ->> 0
                                        else null::text
                                        end as field_value
                             from (select reg.data,
                                          reg.headers,
                                          reg._insert_dttm as create_dttm,
                                          each_outer.key   as field_id,
                                          each_outer.value as field_value,
                                          reg.id
                                   from raw.frm_registration reg,
                                        lateral jsonb_each(reg.data) each_outer(key, value)) t2,
                                  lateral jsonb_each(t2.field_value) each_inner(key, value)) t3
                       group by t3.id, t3.data, t3.headers, t3.field_id) t4
             ) as raw
              left join empty_yandexuid_map fake_uid on fake_uid.id = raw.id
        ;

        insert into stg.tmp_frm_application
        (yandexuid, form_id, passport_uid, passport_login, field_name, field_value, create_dttm)
        select yandexuid, form_id, passport_uid, passport_login, field_name, field_value, create_dttm
        from (
                 select yandexuid,
                        form_id,
                        passport_uid,
                        passport_login,
                        field_name,
                        field_value,
                        create_dttm,
                        row_number() over (partition by yandexuid, form_id, field_name order by create_dttm desc) as rn
                 from stg.stg_frm_application
             ) as stg
        where stg.rn = 1;

        insert into stg.frm_application(yandexuid, form_id, passport_uid, passport_login, field_name, field_value,
                                         create_dttm)
        select yandexuid, form_id, passport_uid, passport_login, field_name, field_value, create_dttm
        from stg.tmp_frm_application
        on conflict (yandexuid, form_id, field_name)
            do update
            set field_value    = excluded.field_value,
                create_dttm    = excluded.create_dttm,
                passport_uid   = excluded.passport_uid,
                passport_login = excluded.passport_login;
    end
$$;
