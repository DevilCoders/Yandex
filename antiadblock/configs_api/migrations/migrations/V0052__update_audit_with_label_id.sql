UPDATE public.audit_log as a SET label_id = c.label_id FROM public.configs as c
WHERE a.params::jsonb ? 'config_id' and a.params->'config_id' = to_jsonb(c.id);
UPDATE public.audit_log as a SET label_id = service_id WHERE NOT a.params::jsonb ? 'config_id';
