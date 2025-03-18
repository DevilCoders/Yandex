--table for storing aabbot config
create table public.bot_chat_config
(
    chat_id              varchar NOT NULL PRIMARY KEY,
    service_id           varchar(127) NOT NULL unique,
    config_notification  bool,
    release_notification bool,
    rules_notification   bool
);

ALTER TABLE ONLY public.bot_chat_config
    ADD CONSTRAINT bot_config_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);


create table public.bot_event
(
    id         BIGSERIAL NOT NULL PRIMARY KEY,
    event_date timestamp without time zone NOT NULL,
    event_type varchar NOT NULL,
    data       jsonb   NOT NULL
);
