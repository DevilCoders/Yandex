UPDATE public.configs SET data = jsonb_set(data, '{CM_TYPE}', '[]'::jsonb) WHERE data::jsonb ? 'CM_TYPE' and data->'CM_TYPE' = '[0]'::jsonb;
