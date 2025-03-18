CREATE TABLE public.checks_in_progress (
    service_id character varying(127) NOT NULL,
    check_id character varying(127) NOT NULL,
    login character varying(63) NOT NULL,
    time_from timestamp without time zone NOT NULL,
    time_to timestamp without time zone NOT NULL,
    PRIMARY KEY (service_id, check_id)
);


ALTER TABLE ONLY public.checks_in_progress
    ADD CONSTRAINT checks_in_progress_fkey FOREIGN KEY (service_id, check_id) REFERENCES public.service_checks(service_id, check_id);