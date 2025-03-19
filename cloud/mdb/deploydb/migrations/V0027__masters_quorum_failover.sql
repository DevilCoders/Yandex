CREATE TABLE deploy.masters_check_view (
   master_id         bigint       NOT NULL,
   checker_fqdn      text         NOT NULL,
   is_alive          boolean      NOT NULL,
   updated_at        timestamptz  NOT NULL,

   CONSTRAINT pk_masters_check_view PRIMARY KEY (master_id, checker_fqdn),

   CONSTRAINT fk_masters_check_view_master_id FOREIGN KEY (master_id)
       REFERENCES deploy.masters ON DELETE CASCADE,

   CONSTRAINT check_fqdn_length CHECK (
       char_length(checker_fqdn) BETWEEN 1 AND 512
   )
);
