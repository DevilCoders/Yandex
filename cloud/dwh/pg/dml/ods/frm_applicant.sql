DO
$$
    BEGIN
        truncate table ods.frm_applicant;
        insert into ods.frm_applicant
        (email, passport_uid, passport_login, mail_marketing, position, first_name, last_name, phone, company, website)
        select email,
               passport_uid,
               passport_login,
               case
                   when mail_marketing in ('yes', 'да', 'Yes', 'Да') then true
                   when mail_marketing is null then null
                   else false
                end as mail_marketing,
               position,
               first_name,
               last_name,
               replace(replace(replace(phone, ' ', ''), '+', ''), '-', '') as phone,
               company,
               site as website
        from (
             select t3.email,
                    t3.create_dttm,
                    first_value(passport_uid) over (partition by email, prt_puid order by create_dttm)     as passport_uid,
                    first_value(passport_login)
                    over (partition by email, prt_plogin order by create_dttm)                             as passport_login,
                    first_value(mail_marketing)
                    over (partition by email, prt_mail_marketing order by create_dttm)                     as mail_marketing,
                    first_value(position)
                    over (partition by email, prt_position order by create_dttm)                           as position,
                    first_value(phone)
                    over (partition by email, prt_phone order by create_dttm)                              as phone,
                    first_value(company)
                    over (partition by email, prt_company order by create_dttm)                            as company,
                    first_value(site) over (partition by email, prt_site order by create_dttm)             as site,
                    first_value(first_name)
                    over (partition by email, prt_first_name order by create_dttm)                         as first_name,
                    first_value(last_name)
                    over (partition by email, prt_last_name order by prt_last_name)                        as last_name,
                    row_number() over (partition by email order by create_dttm desc)                       as rn
             from (
                      select t2.*,
                             sum(case when passport_uid is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_puid,
                             sum(case when passport_login is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_plogin,
                             sum(case when mail_marketing is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_mail_marketing,
                             sum(case when position is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_position,
                             sum(case when phone is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_phone,
                             sum(case when company is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_company,
                             sum(case when site is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_site,
                             sum(case when first_name is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_first_name,
                             sum(case when last_name is null then 0 else 1 end)
                             over (partition by email order by create_dttm) as prt_last_name
                      from (
                               select yandexuid,
                                      form_id,
                                      max(email)                as email,
                                      max(passport_uid)         as passport_uid,
                                      max(passport_login)       as passport_login,
                                      max(first_name)           as first_name,
                                      max(last_name)            as last_name,
                                      max(mail_marketing)       as mail_marketing,
                                      max(position)             as position,
                                      max(phone)                as phone,
                                      max(company)              as company,
                                      max(site)                 as site,
                                      max(create_dttm)          as create_dttm
                               from (
                                        select yandexuid,
                                               passport_uid,
                                               passport_login,
                                               create_dttm,
                                               form_id,
                                               case when field_name = 'firstName' then field_value end                   as first_name,
                                               case when field_name = 'lastName' then field_value end                    as last_name,
                                               case when field_name in ('mail_marketing', 'agreementNewsletters') then field_value end   as mail_marketing,
                                               case when field_name = 'position' then field_value end                    as position,
                                               case when field_name = 'phone' then field_value end                       as phone,
                                               case when field_name = 'email' then lower(field_value) end                as email,
                                               case when field_name = 'company' then field_value end                     as company,
                                               case when field_name = 'site' then lower(field_value) end                 as site
                                        from (
                                                 select yandexuid,
                                                        passport_uid,
                                                        passport_login,
                                                        field_name,
                                                        field_value,
                                                        create_dttm,
                                                        form_id
                                                 from stg.frm_application
                                                 where field_name in
                                                       ('firstName', 'lastName', 'mail_marketing', 'position', 'phone',
                                                        'email', 'company', 'agreementNewsletters', 'site')
                                             ) as t0
                                    ) as t1
                               group by yandexuid, form_id
                        ) as t2
                  ) as t3
             ) as t4
        where t4.rn = 1 and t4.email is not null
        ;
    END
$$;
