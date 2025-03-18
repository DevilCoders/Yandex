UPDATE public.audit_log SET params = jsonb_set(params, '{archived}', 'true'::jsonb) WHERE action::text = 'config_archive';
