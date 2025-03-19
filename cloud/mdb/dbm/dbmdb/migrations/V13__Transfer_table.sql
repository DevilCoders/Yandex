CREATE UNIQUE INDEX uk_containers_dom0_fqdn ON mdb.containers (dom0, fqdn);

CREATE TABLE mdb.transfers (
    id              text NOT NULL,
    src_dom0        text NOT NULL,
    dest_dom0       text NOT NULL,
    container       text NOT NULL,
    placeholder     text NOT NULL,
    started         timestamp with time zone NOT NULL DEFAULT now(),

    CONSTRAINT pk_transfers PRIMARY KEY (id),
    CONSTRAINT fk_transfers_src_dom0_container FOREIGN KEY (src_dom0, container)
        REFERENCES mdb.containers (dom0, fqdn) ON DELETE RESTRICT,
    CONSTRAINT fk_transfers_dest_dom0_placeholder FOREIGN KEY (dest_dom0, placeholder)
        REFERENCES mdb.containers (dom0, fqdn) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_transfers_container ON mdb.transfers(container);
CREATE INDEX i_transfers_src_dom0 ON mdb.transfers(src_dom0);
CREATE INDEX i_transfers_dest_dom0 ON mdb.transfers(dest_dom0);
