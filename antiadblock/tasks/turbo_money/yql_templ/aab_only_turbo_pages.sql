SELECT
    Owner,
    CAST(JSON_QUERY(Yson::SerializeJson(Desktop), "$.ads.id" WITH UNCONDITIONAL WRAPPER) as string) as Blocks
FROM `home/webmaster/prod/export/turbo/turbo-hosts`
WHERE Yson::LookupBool(Desktop, 'aab_enabled')
