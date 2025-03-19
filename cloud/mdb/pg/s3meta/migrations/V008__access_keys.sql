CREATE TABLE s3.access_keys
(
    /*
     * NOTE: Each access key sould belong to a specific user (service_id, uid), not a
     * granted role, as access key is used to authenticate user, while granted roles
     * are used to authorize user. But currently granted role represents "virtual" user
     * with fixed permissions.
     */
    service_id  bigint  NOT NULL,
    user_id     bigint  NOT NULL,
    role        s3.role_type  NOT NULL,

    /*
     * Each access key is uniquely indentified by its ID - a 20-character, alphanumeric
     * string, and has associated access secret key.
     *
     * NOTE: Key's ID is defined as 'text' with separate constraints on its length and
     * contents as 'char(20)' adds space-padding.
     *
     * NOTE: Instead of storing access secret key some versioned secret token is stored.
     * It's version (key_version) determines how to retrieve actual access secret key
     * from the token.
     */
    key_id         text COLLATE "C" NOT NULL,
    secret_token   text COLLATE "C" NOT NULL,
    key_version    integer  NOT NULL,

    issue_date  timestamptz NOT NULL DEFAULT current_timestamp,

    CONSTRAINT pk_access_keys PRIMARY KEY (key_id),

    /*
     * NOTE: When a user's role is removed, we consider this "virtual" user as removed
     * as well and drop all their access keys.
     */
    CONSTRAINT fk_access_keys_service_id_grantee_uid_role_granted_roles
        FOREIGN KEY (service_id, user_id, role)
        REFERENCES s3.granted_roles(service_id, grantee_uid, role)
        ON DELETE CASCADE,

    CONSTRAINT key_id_alnum_length_check CHECK (
        key_id ~ '[[:alnum:]]{20}'
    )
);
