SET TIMEZONE='UTC';

insert into export.crm_mdb_choose_db_test(
yandexuid, form_id, passport_uid, passport_login, create_dttm, agreement_newsletters, email, phone, fio, piiagreement,
project_description, already_use_mdb, ba_id, timezone, lead_source, lead_source_description, utm_campaign, utm_content, utm_medium,
utm_source, utm_term)
select f.yandexuid,
       f.form_id,
       f.passport_uid,
       f.passport_login,
       f.create_dttm,
       f.agreement_newsletters,
       f.email,
       f.phone,
       f.fio,
       f.piiagreement,
       f.project_description,
       case
           when f.already_use_mdb in ('yes', 'да', 'Yes', 'Да') then 1
           when f.already_use_mdb is null then null
           else 0
       end as already_use_mdb,
       coalesce(f.ba_id_forms, f.ba_id_billing) as ba_id,
       f.timezone,
       ls.name      as lead_source,
       fn.form_name as lead_source_description,
       f.utm_campaign,
       f.utm_content,
       f.utm_medium,
       f.utm_source,
       f.utm_term
from (select r.yandexuid,
             r.form_id,
             max(r.passport_uid)         as passport_uid,
             max(r.passport_login)       as passport_login,
             max(r.create_dttm)          as create_dttm,
             max(
                     case
                         when r.field_name::text = 'fio'::text then r.field_value
                         else null::text
                         end)            as fio,
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
                         when r.field_name::text = 'piiagreement'::text then r.field_value
                         else null::text
                         end)            as piiagreement,
             max(
                     case
                         when r.field_name::text = 'project_description'::text then r.field_value
                         else null::text
                         end)            as project_description,
             max(
                     case
                         when r.field_name::text = 'already_use_mdb'::text then r.field_value
                         else null::text
                         end)            as already_use_mdb,
              max(
                     case
                         when r.field_name::text = 'ba_id'::text then
                             case when trim(r.field_value) = ''
                                  then null
                                  else r.field_value
                             end
                         else null::text
                         end)            as ba_id_forms,
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
                         when r.field_name::text = 'utm_term'::text then r.field_value
                         else null::text
                         end)            as utm_term,
             max(cc.timezone)            as timezone,
             max(ba.ba_id)               as ba_id_billing
      from stg.frm_application r
      left join ods.iam_cloud_creator cc
             on r.passport_uid = cc.passport_uid
            and cc.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
      left join ods.bln_billing_account ba
             on r.passport_uid = ba.owner_id
      where r.form_id = 10021710
      group by r.yandexuid, r.form_id
     ) as f
     inner join cdm.dim_form fn on f.form_id = fn.form_id
      left join cdm.dim_lead_source as ls on fn.lead_source_id = ls.id
on conflict (yandexuid, form_id)
do update set passport_uid            = excluded.passport_uid,
              passport_login          = excluded.passport_login,
              create_dttm             = excluded.create_dttm,
              agreement_newsletters   = excluded.agreement_newsletters,
              phone                   = excluded.phone,
              email                   = excluded.email,
              fio                     = excluded.fio,
              piiagreement            = excluded.piiagreement,
              project_description     = excluded.project_description,
              ba_id                   = excluded.ba_id,
              timezone                = excluded.timezone,
              lead_source             = excluded.lead_source,
              lead_source_description = excluded.lead_source_description,
              utm_content             = excluded.utm_content,
              already_use_mdb         = excluded.already_use_mdb,
              utm_source              = excluded.utm_source,
              utm_campaign            = excluded.utm_campaign,
              utm_medium              = excluded.utm_medium,
              utm_term                = excluded.utm_term

;
