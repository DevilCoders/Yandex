SET TIMEZONE='UTC';

insert into ods.site_news_subscribed_email
(email, _insert_dttm)
select email, current_timestamp
  from ods.stg_site_news_subscribed_email
on conflict (email) do nothing

