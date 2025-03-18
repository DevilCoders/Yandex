CREATE TABLE public.service_comments (
    service_id character varying(127) NOT NULL PRIMARY KEY,
    comment text NOT NULL DEFAULT ''
);

ALTER TABLE ONLY public.service_comments
    ADD CONSTRAINT service_comments_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);