$destination_path = {{input1->table_quote()}};


$became_trial = {{ param["became_trial"] -> quote() }};
$common = {{ param["common"] -> quote() }};
$feature_flag_changed = {{ param["feature_flag_changed"] -> quote() }};
$first_paid_consumption = {{ param["first_paid_consumption"] -> quote() }};
$first_payment = {{ param["first_payment"] -> quote() }};
$first_trial_consumption = {{ param["first_trial_consumption"] -> quote() }};
$state_changed = {{ param["state_changed"] -> quote() }};
$cloud_created = {{ param["cloud_created"] -> quote() }};
$create_organization = {{ param["create_organization"] -> quote() }};
$first_service_consumption = {{ param["first_service_consumption"] -> quote() }};

INSERT INTO $destination_path WITH TRUNCATE
SELECT * FROM CONCAT($common, $feature_flag_changed, $first_paid_consumption, $first_payment,
        $first_trial_consumption, $state_changed, $cloud_created, $create_organization, $first_service_consumption,$became_trial)
ORDER BY event_type, event_timestamp, event_entity_id;
