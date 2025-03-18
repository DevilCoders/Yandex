ALTER TABLE ONLY public.audit_log
    DROP CONSTRAINT audit_log_service_id_fkey,
    ADD CONSTRAINT audit_log_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id) ON UPDATE CASCADE;
ALTER TABLE ONLY public.configs
    DROP CONSTRAINT configs_service_id_fkey,
    ADD CONSTRAINT configs_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id) ON UPDATE CASCADE;
ALTER TABLE ONLY public.service_comments
    DROP CONSTRAINT service_comments_service_id_fkey,
    ADD CONSTRAINT service_comments_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id) ON UPDATE CASCADE;
ALTER TABLE ONLY public.service_checks
    DROP CONSTRAINT service_checks_service_id_fkey,
    ADD CONSTRAINT service_checks_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id) ON UPDATE CASCADE;
ALTER TABLE ONLY public.checks_in_progress
    DROP CONSTRAINT checks_in_progress_fkey,
    ADD CONSTRAINT checks_in_progress_fkey FOREIGN KEY (service_id, check_id) REFERENCES public.service_checks(service_id, check_id) ON UPDATE CASCADE;

UPDATE public.services SET id='yandex_player' WHERE id='yastatic.net';
UPDATE public.services SET id='yandex_sport' WHERE id='sport.yandex.ru';
