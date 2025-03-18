ALTER TABLE public.sbs_profiles ADD COLUMN url_settings jsonb NOT NULL DEFAULT '[]'::jsonb;

UPDATE public.sbs_profiles
SET url_settings = subquery.settings
FROM (
SELECT c.settings settings, sbs_profiles.id
FROM sbs_profiles
LEFT JOIN LATERAL(
    SELECT jsonb_agg(
        jsonb_build_object(
            'url', c.urls,
            'selectors', '[]'::jsonb
        )
    ) AS settings
    FROM jsonb_array_elements(sbs_profiles.urls_list) AS c(urls)
) c ON true) as subquery
WHERE sbs_profiles.id = subquery.id;


ALTER TABLE public.sbs_profiles ADD COLUMN general_settings jsonb NOT NULL DEFAULT '{}'::jsonb;
ALTER TABLE public.sbs_profiles DROP COLUMN urls_list;
