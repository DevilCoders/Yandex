ALTER TABLE public.config_statuses ALTER COLUMN status TYPE VARCHAR(63) USING status::text;
ALTER TYPE public.config_status RENAME TO config_status_old;
CREATE TYPE public.config_status AS ENUM('active',
    'test');
ALTER TABLE public.config_statuses ALTER COLUMN status TYPE public.config_status USING status::public.config_status;
DROP TYPE public.config_status_old;
