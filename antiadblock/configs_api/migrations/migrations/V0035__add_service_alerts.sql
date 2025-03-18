CREATE TYPE public.check_state AS ENUM (
    'green',
    'yellow',
    'red'
);

CREATE TABLE public.service_checks (
    service_id character varying(127) NOT NULL,
    group_id character varying(127) NOT NULL,
    check_id character varying(127) NOT NULL,
    state public.check_state NOT NULL,
    value character varying(255) NOT NULL,
    external_url character varying(4096) NOT NULL,
    last_update timestamp without time zone NOT NULL,
    valid_till timestamp without time zone NOT NULL,
    transition_time timestamp without time zone NOT NULL,
    PRIMARY KEY (service_id, check_id)
);


CREATE INDEX idx_service_checks_service_id_check_id ON public.service_checks USING btree (service_id, check_id);
ALTER TABLE ONLY public.service_checks
    ADD CONSTRAINT service_checks_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);
