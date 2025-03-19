SET TIMEZONE='UTC';

insert into export.crm_ba_request
(yandexuid, form_id, passport_uid, passport_login, create_dttm, ba_id, crm_lead_id)
select f.yandexuid,
       f.form_id,
       f.passport_uid,
       f.passport_login,
       f.create_dttm,
       f.ba_id,
       f.crm_lead_id
from (select r.yandexuid,
             r.form_id,
             max(r.passport_uid)         as passport_uid,
             max(r.passport_login::text) as passport_login,
             max(r.create_dttm)          as create_dttm,
             max(
                     case
                         when r.field_name::text = 'l_id'::text then r.field_value
                         else null::text
                         end)            as crm_lead_id,
             max(
                     case
                         when r.field_name::text = 'ba_id'::text then r.field_value
                         else null::text
                         end)            as ba_id
      from stg.frm_application r
      left join ods.iam_cloud_creator cc
             on r.passport_uid = cc.passport_uid
            and cc.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
      left join ods.bln_billing_account ba
             on r.passport_uid = ba.owner_id
      where r.form_id = 10021046 -- ba request form
      group by r.yandexuid, r.form_id
     ) as f
     inner join cdm.dim_form fn on f.form_id = fn.form_id
      left join cdm.dim_lead_source as ls on fn.lead_source_id = ls.id
on conflict (yandexuid, form_id)
    do update set passport_uid            = excluded.passport_uid,
                  passport_login          = excluded.passport_login,
                  create_dttm             = excluded.create_dttm,
                  ba_id                   = excluded.ba_id,
                  crm_lead_id             = excluded.crm_lead_id

;
