ALTER TABLE public.services ALTER COLUMN status TYPE VARCHAR(63) USING status::text;
UPDATE public.services SET status='ok';
ALTER TYPE public.service_status RENAME TO service_status_old;
CREATE TYPE public.service_status AS ENUM (
    'inactive',
    'ok'
);
ALTER TABLE public.services ALTER COLUMN status TYPE public.service_status USING status::public.service_status;
DROP TYPE public.service_status_old;