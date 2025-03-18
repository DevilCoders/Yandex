ALTER TABLE public.sbs_profiles ADD COLUMN tag CHARACTER VARYING(63) NOT NULL DEFAULT 'default';
ALTER TABLE public.sbs_profiles ADD COLUMN is_archived BOOLEAN DEFAULT FALSE;

UPDATE public.sbs_profiles
SET is_archived = TRUE;

UPDATE public.sbs_profiles
SET is_archived = FALSE
WHERE id IN (
    SELECT MAX(id) AS max_id
    FROM public.sbs_profiles
    GROUP BY service_id, tag
);

CREATE INDEX idx_sbs_profiles_service_id_is_archived ON public.sbs_profiles USING btree (service_id, is_archived);
CREATE INDEX idx_sbs_profiles_service_id_tag ON public.sbs_profiles USING btree (service_id, tag);
