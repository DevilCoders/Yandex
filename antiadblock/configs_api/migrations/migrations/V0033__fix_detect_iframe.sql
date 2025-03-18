UPDATE public.configs c1
SET data = jsonb_set(data, '{DETECT_IFRAME}', (data -> 'DETECT_IFRAME') - (SELECT i-1 FROM public.configs c2, jsonb_array_elements(data->'DETECT_IFRAME') with ordinality arr(val, i)
                                                                           WHERE val = '"https://s3.mds.yandex.net/aab-pub/frame.html"' and c2.id = c1.id)::int
                                                || '["https://storage.mds.yandex.net/get-get-frame-content/1731675/frame.html"]'::jsonb)
WHERE data::jsonb ? 'DETECT_IFRAME' AND data -> 'DETECT_IFRAME' @> '["https://s3.mds.yandex.net/aab-pub/frame.html"]'::jsonb;