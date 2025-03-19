create view v_dim_lead
            (id, first_name, last_name, passport_uid, email, mail_tech, mail_feature, mail_support, mail_billing,
             mail_promo, mail_testing, phone, timezone, mail_info, mail_event, mail_technical, language, position,
             website, company, mail_news, mail_marketing)
as
select dim_lead.id,
       dim_lead.first_name,
       dim_lead.last_name,
       dim_lead.passport_uid,
       dim_lead.email,
       dim_lead.mail_tech,
       dim_lead.mail_feature,
       dim_lead.mail_support,
       dim_lead.mail_billing,
       dim_lead.mail_promo,
       dim_lead.mail_testing,
       dim_lead.phone,
       dim_lead.timezone,
       dim_lead.mail_info,
       dim_lead.mail_event,
       dim_lead.mail_technical,
       dim_lead.language,
       dim_lead."position",
       dim_lead.website,
       dim_lead.company,
       dim_lead.mail_news,
       dim_lead.mail_marketing
from cdm.dim_lead;

grant select on export.v_dim_lead to "etl-user";

