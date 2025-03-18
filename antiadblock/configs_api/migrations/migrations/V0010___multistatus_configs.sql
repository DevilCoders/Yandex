CREATE TABLE public.config_statuses (
    config_id BIGINT,
    status public.config_status NOT NULL,
    PRIMARY KEY(config_id, status)
);

INSERT INTO public.config_statuses(config_id, status) SELECT id, status FROM configs WHERE status != 'inactive';
INSERT INTO public.config_statuses(config_id, status) SELECT id, 'test' FROM configs where status = 'active' and service_id in (SELECT service_id FROM configs GROUP BY service_id HAVING every(status != 'test'));

ALTER TABLE configs DROP COLUMN status;
