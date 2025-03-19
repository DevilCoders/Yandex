create view ods.v_frm_registration
(yandexuid, form_id, passport_uid, passport_login, field_name, field_value, created_dttm) as
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
       t4.created_dttm
from (select ((t3.headers -> 'X-Cookies'::text) ->> 'yandexuid'::text)::varchar(255) as yandexuid,
             (t3.headers ->> 'X-Form-Id'::text)::bigint                        as form_id,
             t3.headers ->> 'X-Passport-Uid'::text                             as passport_uid,
             t3.headers ->> 'X-Passport-Login'::text                           as passport_login,
             max(t3.field_name)                                                as field_name,
             max(t3.field_value::character varying::text)                      as field_value,
             max(t3.created_time)                                              as created_dttm
      from (select t2.data,
                   t2.headers,
                   t2.field_id,
                   t2.created_time,
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
                         reg._insert_dttm as created_time,
                         each_outer.key   as field_id,
                         each_outer.value as field_value
                  from raw.frm_registration reg,
                       lateral jsonb_each(reg.data) each_outer(key, value)) t2,
                 lateral jsonb_each(t2.field_value) each_inner(key, value)) t3
      group by t3.data, t3.headers, t3.field_id) t4;

grant select on ods.v_frm_registration to "etl-user";
