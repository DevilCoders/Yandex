insert into meta.src_to_raw_main(connection_id, object_id, source_table_name, target_table_name, write_mode)
select
    'backoffice' as connection_id,
    'bo_' || t.table_name as object_id,
    t.table_name as source_table_name,
    'yt:///home/cloud_analytics/dwh/raw/backoffice/' || t.table_name,
    'append' as write_mode
 from (
    select
       unnest(array['applications', 'blog_post_locales', 'blog_posts', 'blog_posts_likes', 'blog_posts_tags',
                    'blog_tag_locales', 'blog_tags', 'case_categories', 'cases', 'cases_services', 'cities',
                      'event_reminders', 'event_speakers', 'events', 'events_services', 'features', 'features_services',
                      'incident_comments', 'incidents', 'incidents_services', 'incidents_zones', 'knex_migrations',
                      'knex_migrations_lock', 'page_locales', 'page_versions', 'pages', 'participants', 'places',
                      'services', 'services_zones', 'speakers', 'speeches', 'speeches_speakers', 'votes', 'zones']) as table_name
     ) as t
