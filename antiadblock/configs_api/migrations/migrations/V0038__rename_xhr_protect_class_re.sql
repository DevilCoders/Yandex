UPDATE public.configs SET data = jsonb_set(data, '{VISIBILITY_PROTECTION_CLASS_RE}', data->'XHR_PROTECT_CLASS_RE') WHERE (data::jsonb ? 'XHR_PROTECT_CLASS_RE');
UPDATE public.configs SET data = jsonb_set(data, '{XHR_PROTECT}', to_jsonb(true)) WHERE (data::jsonb ? 'XHR_PROTECT_CLASS_RE');
UPDATE public.configs SET data = data - 'XHR_PROTECT_CLASS_RE';