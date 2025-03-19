SET TIMEZONE='UTC';

DO
$$
    BEGIN
       insert into cdm.dim_lead
        (first_name, last_name, email, phone, position, company, mail_marketing, email_other,
         _insert_dttm, _update_dttm)
        select apl.first_name,
               apl.last_name,
               apl.email,
               apl.phone,
               apl.position,
               apl.company,
               apl.mail_marketing,
               apl.email,
               now(),
               now()
        from ods.other_applicant apl
        where not exists(
            select 1
            from cdm.dim_lead l
            where l.email = apl.email
        );

        update cdm.dim_lead l
           set
               first_name     = coalesce(apl.first_name, l.first_name),
               last_name      = coalesce(apl.last_name, l.last_name),
               phone          = coalesce(apl.phone, l.phone),
               position       = coalesce(apl.position, l.position),
               company        = coalesce(apl.company, l.company),
               mail_marketing = coalesce(apl.mail_marketing, l.mail_marketing),
               email_other    = apl.email,
               _update_dttm   = now()
            from ods.other_applicant apl
           where l.email = apl.email
             and (
                 coalesce(apl.phone, '') <> coalesce(l.phone, '') or
                 coalesce(apl.first_name, '') <> coalesce(l.first_name, '') or
                 coalesce(apl.last_name, '') <> coalesce(l.last_name, '') or
                 coalesce(apl.position, '') <> coalesce(l.position, '') or
                 coalesce(apl.company, '') <> coalesce(l.company, '') or
                 coalesce(apl.mail_marketing::varchar, '') <> coalesce(l.mail_marketing::varchar, '') or
                 apl.email <> coalesce(l.email_other, '')
             )
        ;
    END
$$;
