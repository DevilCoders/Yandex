CREATE TABLE s3.tvm2_granted_roles
(
    service_id   bigint  NOT NULL,
    role         s3.role_type  NOT NULL,
    grantee_uid  bigint  NOT NULL,
    issue_date   timestamp with time zone  NOT NULL DEFAULT current_timestamp,
    CONSTRAINT pk_tvm2_granted_roles PRIMARY KEY (service_id, role, grantee_uid),
    CONSTRAINT fk_tvm2_granted_roles_service_id_accounts FOREIGN KEY (service_id)
        REFERENCES s3.accounts ON DELETE RESTRICT
);
