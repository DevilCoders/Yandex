UPDATE public.configs SET data = jsonb_set(data, '{CM_TYPE}', jsonb_build_array(data->'CM_TYPE')) WHERE data::jsonb ? 'CM_TYPE' and data->'CM_TYPE'::text IN ('0', '1', '2');
UPDATE public.configs SET data = jsonb_set(data, '{CM_TYPE}', jsonb_build_array(1, 2)) WHERE data::jsonb ? 'CM_TYPE' and data->'CM_TYPE'::text = '3';
