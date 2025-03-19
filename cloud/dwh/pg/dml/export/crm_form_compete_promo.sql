SET TIMEZONE='UTC';

insert into export.crm_form_compete_promo
(yandexuid, form_id, passport_uid, passport_login, create_dttm, company, agreement_newsletters, phone, email, message, last_name,
first_name, "check", utm_content, utm_source, utm_campaign, utm_medium, timezone, ba_id, lead_source, lead_source_description)
select f.yandexuid,
       f.form_id,
       f.passport_uid,
       f.passport_login,
       f.create_dttm,
       f.company,
       f.agreement_newsletters,
       f.phone,
       f.email,
       f.message,
       f.last_name,
       f.first_name,
       f.check,
       f.utm_content,
       f.utm_source,
       f.utm_campaign,
       f.utm_medium,
       f.timezone,
       f.ba_id,
       ls.name                                                                  as lead_source,
       -- for ISV
       fn.form_name as lead_source_description
from (select r.yandexuid,
             r.form_id,
             max(r.passport_uid)         as passport_uid,
             max(r.passport_login::text) as passport_login,
             max(r.create_dttm)          as create_dttm,
             max(
                     case
                         when r.field_name::text = 'company'::text then r.field_value
                         else null::text
                         end)            as company,
             max(
                     case
                         when r.field_name::text = 'agreementNewsletters'::text then r.field_value
                         else null::text
                         end)            as agreement_newsletters,
             max(
                     case
                         when r.field_name::text = 'phone'::text then r.field_value
                         else null::text
                         end)            as phone,
             max(
                     case
                         when r.field_name::text = 'email'::text then r.field_value
                         else null::text
                         end)            as email,
             max(
                     case
                         when r.field_name::text = 'lastName'::text then r.field_value
                         else null::text
                         end)            as last_name,
             max(
                     case
                         when r.field_name::text = 'firstName'::text then r.field_value
                         else null::text
                         end)            as first_name,
             max(
                     case
                         when r.field_name::text = 'utm_content'::text then r.field_value
                         else null::text
                         end)            as utm_content,
             max(
                     case
                         when r.field_name::text = 'utm_source'::text then r.field_value
                         else null::text
                         end)            as utm_source,
             max(
                     case
                         when r.field_name::text = 'utm_campaign'::text then r.field_value
                         else null::text
                         end)            as utm_campaign,
             max(
                     case
                         when r.field_name::text = 'utm_medium'::text then r.field_value
                         else null::text
                         end)            as utm_medium,
             max(
                     case
                         when r.field_name::text in ('comment'::text, 'message'::text) then r.field_value
                         else null::text
                         end)            as message,
              max(
                     case
                         when r.field_name::text in ('check'::text, 'message'::text) then r.field_value
                         else null::text
                         end)            as "check",
             max(cc.timezone)            as timezone,
             max(ba.ba_id)               as ba_id
      from stg.frm_application r
      left join ods.iam_cloud_creator cc
             on r.passport_uid = cc.passport_uid
            and cc.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
      left join ods.bln_billing_account ba
             on r.passport_uid = ba.owner_id
      where r.form_id = 10018095 -- compete promo
      group by r.yandexuid, r.form_id
     ) as f
     inner join cdm.dim_form fn on f.form_id = fn.form_id
      left join cdm.dim_lead_source as ls on fn.lead_source_id = ls.id
on conflict (yandexuid, form_id)
    do update set passport_uid            = excluded.passport_uid,
                  passport_login          = excluded.passport_login,
                  create_dttm             = excluded.create_dttm,
                  message                 = excluded.message,
                  company                 = excluded.company,
                  agreement_newsletters   = excluded.agreement_newsletters,
                  phone                   = excluded.phone,
                  email                   = excluded.email,
                  last_name               = excluded.last_name,
                  first_name              = excluded.first_name,
                  "check"                 = excluded.check,
                  utm_content             = excluded.utm_content,
                  lead_source             = excluded.lead_source,
                  lead_source_description = excluded.lead_source_description
;
