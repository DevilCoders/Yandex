SET TIMEZONE='UTC';

DO
$$
    declare
	    current_ts timestamp with time zone;
    BEGIN
        current_ts := current_timestamp;
        with inserts
        (timezone, mail_tech, mail_feature, mail_support, mail_billing, mail_promo, mail_testing, mail_marketing,
         mail_info, mail_event, mail_technical, user_settings_email, email, login, passport_uid, first_name,
         last_name, user_settings_language, phone, valid_from, valid_to) as (
            select distinct cc1.timezone,
                            cc1.mail_tech,
                            cc1.mail_feature,
                            cc1.mail_support,
                            cc1.mail_billing,
                            cc1.mail_promo,
                            cc1.mail_testing,
                            cc1.mail_marketing,
                            cc1.mail_info,
                            cc1.mail_event,
                            cc1.mail_technical,
                            lower(cc1.user_settings_email),
                            cc1.email,
                            cc1.login,
                            cc1.passport_uid,
                            cc1.first_name,
                            cc1.last_name,
                            cc1.user_settings_language,
                            cc1.phone,
                            current_ts                               as valid_from,
                            to_timestamp('9999-12-31', 'YYYY-MM-DD') as valid_to
            from stg.iam_cloud_creator cc1
            where not exists(
                select 1 from ods.iam_cloud_creator cc0
                 where cc0.passport_uid = cc1.passport_uid
            )
            union
            select distinct cc0.timezone,
                            cc0.mail_tech,
                            cc0.mail_feature,
                            cc0.mail_support,
                            cc0.mail_billing,
                            cc0.mail_promo,
                            cc0.mail_testing,
                            cc0.mail_marketing,
                            cc0.mail_info,
                            cc0.mail_event,
                            cc0.mail_technical,
                            lower(cc0.user_settings_email),
                            cc0.email,
                            cc0.login,
                            cc0.passport_uid,
                            cc0.first_name,
                            cc0.last_name,
                            cc0.user_settings_language,
                            cc0.phone,
                            current_ts                               as valid_from,
                            to_timestamp('9999-12-31', 'YYYY-MM-DD') as valid_to
            from stg.iam_cloud_creator cc0
            where exists(
                  select 1
                  from ods.iam_cloud_creator cc1
                  where cc1.passport_uid = cc0.passport_uid
                    and cc1.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
                    and (
                          coalesce(cc1.timezone, '') <> coalesce(cc0.timezone, '') or
                          coalesce(cc1.mail_tech::varchar, '') <> coalesce(cc0.mail_tech::varchar, '') or
                          coalesce(cc1.mail_feature::varchar, '') <> coalesce(cc0.mail_feature::varchar, '') or
                          coalesce(cc1.mail_support::varchar, '') <> coalesce(cc0.mail_support::varchar, '') or
                          coalesce(cc1.mail_billing::varchar, '') <> coalesce(cc0.mail_billing::varchar, '') or
                          coalesce(cc1.mail_promo::varchar, '') <> coalesce(cc0.mail_promo::varchar, '') or
                          coalesce(cc1.mail_testing::varchar, '') <> coalesce(cc0.mail_testing::varchar, '') or
                          coalesce(cc1.mail_marketing::varchar, '') <> coalesce(cc0.mail_marketing::varchar, '') or
                          coalesce(cc1.mail_info::varchar, '') <> coalesce(cc0.mail_info::varchar, '') or
                          coalesce(cc1.mail_event::varchar, '') <> coalesce(cc0.mail_event::varchar, '') or
                          coalesce(cc1.mail_technical::varchar, '') <> coalesce(cc0.mail_technical::varchar, '') or
                          coalesce(cc1.user_settings_email, '') <> coalesce(lower(cc0.user_settings_email), '') or
                          coalesce(cc1.email, '') <> coalesce(cc0.email, '') or
                          coalesce(cc1.login, '') <> coalesce(cc0.login, '') or
                          coalesce(cc1.first_name, '') <> coalesce(cc0.first_name, '') or
                          coalesce(cc1.last_name, '') <> coalesce(cc0.last_name, '') or
                          coalesce(cc1.user_settings_language, '') <> coalesce(cc0.user_settings_language, '') or
                          coalesce(cc1.phone, '') <> coalesce(cc0.phone, '')
                      )
              )
        ),
        updates(passport_uid) as (
            -- expire
            update ods.iam_cloud_creator cc1
               set valid_to = current_ts - interval '1 second'
              from inserts
             where cc1.valid_to = to_timestamp('9999-12-31', 'YYYY-MM-DD')
               and cc1.passport_uid = inserts.passport_uid
            returning cc1.passport_uid
        )
        insert into ods.iam_cloud_creator
        select * from inserts
        ;
    END
$$;
