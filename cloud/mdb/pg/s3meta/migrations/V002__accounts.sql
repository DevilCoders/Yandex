CREATE TYPE s3.account_status_type AS ENUM (
    'active',
    'suspended'
);

CREATE TABLE s3.accounts
(
    service_id         bigint  NOT NULL,
    status             s3.account_status_type  NOT NULL DEFAULT 'active',
    registration_date  timestamp with time zone  NOT NULL DEFAULT current_timestamp,
    CONSTRAINT pk_accounts PRIMARY KEY (service_id)
);


/*
 * Roles:
 * - admin:
 *     Has full control of the service account's objects and buckets.
 */
CREATE TYPE s3.role_type AS ENUM (
    'admin'
);

CREATE TABLE s3.granted_roles
(
    service_id   bigint  NOT NULL,
    role         s3.role_type  NOT NULL,
    grantee_uid  bigint  NOT NULL,
    issue_date   timestamp with time zone  NOT NULL DEFAULT current_timestamp,
    CONSTRAINT pk_granted_roles PRIMARY KEY (service_id, role, grantee_uid),
    CONSTRAINT fk_granted_roles_service_id_accounts FOREIGN KEY (service_id)
        REFERENCES s3.accounts ON DELETE RESTRICT
);
