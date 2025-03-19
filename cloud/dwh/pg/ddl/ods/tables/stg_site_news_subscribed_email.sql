create table ods.stg_site_news_subscribed_email
(
    email        varchar(1024),
    _insert_dttm timestamp with time zone default timezone('utc'::text, now())
);
