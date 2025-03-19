
CREATE TYPE cms.decision_status AS ENUM (
    'ok',
    'rejected',
    'escalated'
);

CREATE TABLE cms.decisions (
    id          bigint                    GENERATED ALWAYS AS IDENTITY NOT NULL,
    request_id  bigint                    NOT NULL,
    status      cms.decision_status       NOT NULL,
    explanation text                      NOT NULL,
    decided_at  timestamp with time zone  NOT NULL DEFAULT now(),

    CONSTRAINT pk_decisions PRIMARY KEY (id),
    CONSTRAINT uk_decision_request_id UNIQUE (request_id),
    CONSTRAINT decide_with_explanation CHECK (
        explanation != '' OR status NOT IN ('ok', 'rejected', 'escalated')
    ),
    CONSTRAINT fk_decisions_to_requests FOREIGN KEY (request_id)
        REFERENCES cms.requests (id) ON DELETE CASCADE
);

ALTER TABLE cms.requests RENAME CONSTRAINT
    at_least_one_fqnd TO at_least_one_fqdn;

