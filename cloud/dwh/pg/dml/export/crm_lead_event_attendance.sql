SET TIMEZONE='UTC';

insert into export.crm_lead_event_attendance
(lead_id, event_id, first_name, last_name, email, phone, company, position, ba_id, mail_marketing, needs_call,
 registration_status, visited, event_date, event_name, event_link, event_form_id, event_city, event_is_online,
 lead_source, cloud_interest_reason, created_at, lead_source_description)
select   l.id                     as lead_id,
         e.id                     as event_id,
         l.first_name,
         l.last_name,
         l.email,
         l.phone,
         l.company,
         l."position",
         ba.ba_id,
         l.mail_marketing::integer as mail_marketing,
         le.needs_call::integer    as needs_call,
         le.registration_status,
         le.visited::integer       as visited,
         e.date                    as event_date,
         e.name_ru                 as event_name,
         e.url                     as event_link,
         e.registration_form_id    as event_form_id,
         e.city_ru                 as event_city,
         e.is_online::integer      as event_is_online,
         'Events'::text            as lead_source,
         le.cloud_interest_reason  as cloud_interest_reason,
         le.created_at,
         case
            when (coalesce(le.needs_call, true) = true and le.cloud_interest_reason is not null and
                  le.cloud_interest_reason not in ('Другое', 'Затрудняюсь ответить'))
                then 'Consult Call'::text
             else 'Test Call'::text
         end as lead_source_description
    from cdm.dim_lead l
    join cdm.fct_lead_event_attendance le on le.lead_id = l.id
    join cdm.dim_event e on e.id = le.event_id
    left join (select max(bln_billing_account.ba_id::text) as ba_id,
                     bln_billing_account.owner_id
              from ods.bln_billing_account
              group by bln_billing_account.owner_id) ba on l.passport_uid = ba.owner_id
    where e.date >= to_timestamp('2020-07-08'::text, 'YYYY-MM-dd'::text)
      and e.date < CURRENT_DATE
      and (
        le.needs_call    = true or
        (coalesce(le.needs_call, true) = true and
        le.cloud_interest_reason is not null and
        le.cloud_interest_reason not in ('Другое', 'Затрудняюсь ответить'))
      )
on conflict (lead_id, event_id)
    do nothing
;
