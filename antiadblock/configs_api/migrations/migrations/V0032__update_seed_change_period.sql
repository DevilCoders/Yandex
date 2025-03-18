UPDATE public.configs SET data = jsonb_set(data, '{SEED_CHANGE_PERIOD}', '6'::jsonb) WHERE NOT(data::jsonb ? 'SEED_CHANGE_PERIOD') OR data -> 'SEED_CHANGE_PERIOD' = '1'::jsonb;
