PRAGMA Library("datetime.sql");
IMPORT `datetime` SYMBOLS $get_msk_datetime;

-- Input
$visits_folder = {{ param["visits_folder"] -> quote() }};
$billing_accounts_table = {{ param["billing_accounts_table"] -> quote() }};
$applications_table = {{ param["applications_table"] -> quote() }};
$participants_table = {{ param["participants_table"] -> quote() }};
$common_ba_events_table = {{ param["billing_account_common_events_table"] -> quote() }};
$first_trial_consumption_table = {{ param["first_trial_consumption_table"] -> quote() }};
$first_paid_consumption_table = {{ param["first_paid_consumption_table"] -> quote() }};

-- Output
$destination_path = {{input1 -> table_quote()}};

$first_ba_by_puid = (
    SELECT
        MIN_BY(billing_account_id, created_at)  AS billing_account_id,
        puid                                    AS puid
    FROM $billing_accounts_table
    WHERE owner_passport_uid IS NOT NULL
    GROUP BY owner_passport_uid AS puid
);

$applications_with_puid = (
    SELECT
        CAST(participants.puid AS String)   AS puid,
        CAST(applications.id   AS String)   AS event_id,
        applications.created_at_msk         AS event_time_msk,
        applications.visited                AS visited,
    FROM $applications_table                AS applications
    JOIN $participants_table                AS participants
    ON participants.id == applications.participant_id
);

$event_applications = (  -- without ba_id yet
    SELECT
        puid                            AS puid,
        event_id                        AS event_id,
        event_time_msk                  AS event_time_msk,
        'event_application'             AS event_type
    FROM $applications_with_puid
);

$event_visitions = ( -- without ba_id yet
    SELECT
        puid               AS puid,
        event_id           AS event_id,
        event_time_msk     AS event_time_msk,  -- can we find time of visition (not application)?
        'event_visit'      AS event_type
    FROM $applications_with_puid
    WHERE visited
);

$site_visits = (  -- without ba_id yet
    SELECT
        CAST(puid     AS String)                AS puid,
        'site_visit'                            AS event_type,
        CAST(visit_id AS String)                AS event_id,
        $get_msk_datetime(event_start_dt_utc)   AS event_time_msk
    FROM RANGE($visits_folder)
);

$first_ba_creations = (  -- without puid yet
    SELECT
        event_id                              AS event_id,
        $get_msk_datetime(event_timestamp)    AS event_time_msk,
        event_type                            AS event_type,
        billing_account_id                    AS billing_account_id
    FROM $common_ba_events_table
    WHERE event_type = "billing_account_created"
);

$first_trial_consumptions = (  -- without puid yet
    SELECT
        event_id                              AS event_id,
        $get_msk_datetime(event_timestamp)    AS event_time_msk,
        event_type                            AS event_type,
        billing_account_id                    AS billing_account_id
    FROM $first_trial_consumption_table
);

$first_paid_consumptions = (  -- without puid yet
    SELECT
        event_id                              AS event_id,
        $get_msk_datetime(event_timestamp)    AS event_time_msk,
        event_type                            AS event_type,
        billing_account_id                    AS billing_account_id
    FROM $first_paid_consumption_table
);

$enriched_with_ba_id = (
    SELECT
        events.event_id             AS event_id,
        events.event_time_msk       AS event_time_msk,
        events.event_type           AS event_type,
        events.puid                 AS puid,
        first_ba.billing_account_id AS billing_account_id
    FROM (
        SELECT * FROM $site_visits
        UNION ALL
        SELECT * FROM $event_applications
        UNION ALL
        SELECT * FROM $event_visitions
    ) AS events
    LEFT JOIN $first_ba_by_puid AS first_ba
    USING (puid)
);

$enriched_with_puid = (
    SELECT
        events.event_id             AS event_id,
        events.event_time_msk       AS event_time_msk,
        events.event_type           AS event_type,
        first_ba.puid               AS puid,
        events.billing_account_id   AS billing_account_id
    FROM (
        SELECT * FROM $first_ba_creations
        UNION ALL
        SELECT * FROM $first_trial_consumptions
        UNION ALL
        SELECT * FROM $first_paid_consumptions
    ) AS events
    JOIN $first_ba_by_puid AS first_ba
    USING (billing_account_id)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT * FROM $enriched_with_ba_id
UNION ALL
SELECT * FROM $enriched_with_puid
ORDER BY event_type, event_id
