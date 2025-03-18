CREATE TABLE public.autoredirect_services (
    id BIGSERIAL PRIMARY KEY,
    webmaster_service_id character varying(512) NOT NULL UNIQUE,
    domain character varying(512) NOT NULL UNIQUE
);
