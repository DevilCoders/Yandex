$billing_accounts_history_table = {{ param["billing_accounts_history_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$banned_unblocked = (
    SELECT * FROM (
        SELECT
            billing_account_id,
            updated_at,
            is_suspended_by_antifraud,
            LEAD (is_suspended_by_antifraud) OVER w AS still_blocked
        FROM $billing_accounts_history_table
        WINDOW w AS (
            PARTITION BY billing_account_id
            ORDER BY updated_at
        )
    )
    WHERE is_suspended_by_antifraud AND NOT still_blocked
);

INSERT INTO $dst_table WITH TRUNCATE
                                SELECT billing_account_id,
                                       updated_at AS blocked_at
                                FROM $banned_unblocked
                                ORDER BY billing_account_id ASC
;
