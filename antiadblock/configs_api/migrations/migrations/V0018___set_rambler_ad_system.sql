UPDATE public.configs SET data = jsonb_set(data, '{AD_SYSTEMS}', '[1, 3, 4]'::jsonb) WHERE service_id IN ('livejournal', 'rambler_horoscopes');
