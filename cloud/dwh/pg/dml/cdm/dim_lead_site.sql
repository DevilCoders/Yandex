SET TIMEZONE='UTC';

DO
$$
    BEGIN
       insert into cdm.dim_lead
      (email, mail_news, _insert_dttm, _update_dttm)
       select e.email,
              true,
              now(),
              now()
        from ods.site_news_subscribed_email e
        where not exists(
            select 1
              from cdm.dim_lead l
             where l.email = e.email
        );

        update cdm.dim_lead l
           set mail_news =  true,
               _update_dttm = now()
          from ods.site_news_subscribed_email e
         where l.email = e.email
           and l.mail_news <> true
        ;
    END
$$;
