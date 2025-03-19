SET TIMEZONE='UTC';

do
$$
    begin
        insert into cdm.dim_lead
        (first_name, last_name, passport_uid, email, mail_tech, mail_feature, mail_support,
         mail_billing, mail_promo, mail_testing, mail_info, mail_event, mail_technical, phone, language, timezone,
         email_iam, _insert_dttm, _update_dttm)
        select first_name,
               last_name,
               passport_uid,
               user_settings_email,
               mail_tech,
               mail_feature,
               mail_support,
               mail_billing,
               mail_promo,
               mail_testing,
               mail_info,
               mail_event,
               mail_technical,
               phone,
               user_settings_language,
               timezone,
               cc.user_settings_email,
               now(),
               now()
        from (
                 -- users may have different passport ids but the same user_settings_email
                 select c.*, row_number() over (partition by user_settings_email order by 1) as rn
                 from ods.iam_cloud_creator c
                 where c.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
             ) cc
        where cc.rn = 1
          and not exists(
                select 1
                from cdm.dim_lead l
                where l.email = cc.user_settings_email
          );

        update cdm.dim_lead l
        set first_name     = cc.first_name,
            last_name      = cc.last_name,
            mail_tech      = cc.mail_tech,
            mail_feature   = cc.mail_feature,
            mail_support   = cc.mail_support,
            mail_billing   = cc.mail_billing,
            mail_promo     = cc.mail_promo,
            mail_testing   = cc.mail_testing,
            mail_info      = cc.mail_info,
            mail_event     = cc.mail_event,
            mail_technical = cc.mail_technical,
            phone          = cc.phone,
            language       = cc.user_settings_language,
            timezone       = cc.timezone,
            email_iam      = cc.user_settings_email,
            _update_dttm   = now()
        from (
             -- users may have different passport ids but the same user_settings_email
             select c.*, row_number() over (partition by user_settings_email order by 1) as rn
               from ods.iam_cloud_creator c
              where c.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
         ) cc
         where l.email = cc.email
          and (
                coalesce(l.first_name, '') <> coalesce(cc.first_name, '') or
                coalesce(l.last_name, '') <> coalesce(cc.last_name, '') or
                coalesce(l.mail_tech::varchar, '') <> coalesce(cc.mail_tech::varchar, '') or
                coalesce(l.mail_feature::varchar, '') <> coalesce(cc.mail_feature::varchar, '') or
                coalesce(l.mail_support::varchar, '') <> coalesce(cc.mail_support::varchar, '') or
                coalesce(l.mail_billing::varchar, '') <> coalesce(cc.mail_billing::varchar, '') or
                coalesce(l.mail_promo::varchar, '') <> coalesce(cc.mail_promo::varchar, '') or
                coalesce(l.mail_testing::varchar, '') <> coalesce(cc.mail_testing::varchar, '') or
                coalesce(l.mail_info::varchar, '') <> coalesce(cc.mail_info::varchar, '') or
                coalesce(l.mail_event::varchar, '') <> coalesce(cc.mail_event::varchar, '') or
                coalesce(l.mail_technical::varchar, '') <> coalesce(cc.mail_technical::varchar, '') or
                coalesce(l.phone::varchar, '') <> coalesce(cc.phone::varchar, '') or
                coalesce(l.language::varchar, '') <> coalesce(cc.user_settings_language::varchar, '') or
                coalesce(l.timezone::varchar, '') <> coalesce(cc.timezone::varchar, '') or
                coalesce(l.email_iam::varchar, '') <> coalesce(cc.user_settings_email::varchar, '')
          )
        ;
    end
$$;


