SET TIMEZONE='UTC';

DO
$$
    BEGIN
        truncate table ods.bcf_applicant;
        insert into ods.bcf_applicant
        (email, participant_id, created_at, first_name, last_name, company, position, phone, website, has_cloud_account, mail_marketing, passport_uid, _insert_dttm)
        select t5.email,
               t5.participant_id,
               t5.created_at,
               t5.first_name,
               t5.last_name,
               t5.company,
               t5.position,
               t5.phone,
               t5.website,
               t5.has_cloud_account,
               t5.mail_marketing,
               p.uid as passport_uid,
               now()
        from (
                 select email,
                        first_value(participant_id) over w       as participant_id,
                        first_value(created_at) over w           as created_at,
                        first_value(first_name) over w           as first_name,
                        first_value(last_name) over w            as last_name,
                        first_value(company) over w              as company,
                        first_value(position) over w             as position,
                        first_value(phone) over w                as phone,
                        first_value(website) over w              as website,
                        first_value(has_cloud_account) over w    as has_cloud_account,
                        first_value(mail_marketing) over w       as mail_marketing,
                        row_number() over w                      as rn
                 from (
                          select participant_id,
                                 max(created_at)                                                  as created_at,
                                 max(first_name)                                                  as first_name,
                                 max(last_name)                                                   as last_name,
                                 max(company)                                                     as company,
                                 max(position)                                                    as position,
                                 replace(replace(replace(max(phone), ' ', ''), '+', ''), '-', '') as phone,
                                 max(email)                                                       as email,
                                 max(website)                                                     as website,
                                 max(has_cloud_account) = 1                                       as has_cloud_account,
                                 case
                                     when 'Нет' = any (array_agg(maybe_mail_marketing)) or
                                          'нет' = any (array_agg(maybe_mail_marketing)) or
                                          'no' = any (array_agg(maybe_mail_marketing)) or
                                          'No' = any (array_agg(maybe_mail_marketing))
                                         then false
                                     else true
                                     end                                                          as mail_marketing
                          from (
                                   select application_id,
                                          participant_id,
                                          first_name,
                                          last_name,
                                          company,
                                          position,
                                          phone,
                                          email,
                                          website,
                                          created_at,
                                          maybe_mail_marketing,
                                          case
                                              when has_cloud_account is null then null
                                              when has_cloud_account in ('yes', 'да', 'Yes', 'Да') then 1
                                              else 0
                                              end as has_cloud_account
                                   from (
                                            select application_id,
                                                   participant_id,
                                                   created_at,
                                                   case when field_slug = 'name' then field_value end               as first_name,
                                                   case when field_slug = 'last_name' then field_value end          as last_name,
                                                   case when field_slug = 'work' then field_value end               as company,
                                                   case when field_slug = 'position' then field_value end           as position,
                                                   case when field_slug = 'phone' then field_value end              as phone,
                                                   case when field_slug = 'email' then lower(field_value) end       as email,
                                                   case when field_slug = 'website' then lower(field_value) end     as website,
                                                   case when field_slug = 'user_status' then lower(field_value) end as has_cloud_account,
                                                   case when field_name = '<p>' then lower(field_value) end         as maybe_mail_marketing
                                            from (
                                                     select app.id as application_id,
                                                            app.participant_id,
                                                            app.field_value,
                                                            app.field_name,
                                                            app.field_slug,
                                                            app.created_at
                                                     from stg.bcf_application app
                                                     where field_slug in
                                                           ('name', 'last_name', 'work', 'position',
                                                            'phone',
                                                            'email', 'website', 'user_status')
                                                        or field_name = '<p>'
                                                 ) as t
                                        ) as tt
                               ) as t3
                          group by t3.application_id, t3.participant_id
                      ) as t4
                     window w as (partition by participant_id, email order by created_at desc)
             ) as t5
                 join stg.bcf_participant p
                      on p.id = t5.participant_id
        where t5.rn = 1;
    END
$$;
