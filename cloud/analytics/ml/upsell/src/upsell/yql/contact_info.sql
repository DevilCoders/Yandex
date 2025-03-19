USE hahn;

$res_table = '//home/cloud_analytics/import/crm/leads/contact_info';
$to_title = ($string) -> {
    return Cast(Unicode::ToTitle(Cast(String::AsciiToTitle($string) AS Utf8)) AS String)
};

INSERT INTO $res_table WITH TRUNCATE
    SELECT
        billing_account_id,
        Coalesce(first_name || ' ' || middle_name || ' ' || last_name, first_name || ' ' || last_name,  longname, name) AS display_name,
        first_name,
        Coalesce(last_name, name) AS last_name,
        email,
        Re2::Replace('[+\\-\\s\\(\\)]')(phone, '') AS phone,
        inn
    FROM (
        SELECT
            billing_account_id,
            original_type,
            person_data,
            Yson::ConvertToString(person_data[original_type]['email']) AS email,
            Yson::ConvertToString(person_data[original_type]['phone']) AS phone,
            $to_title(Yson::ConvertToString(person_data[original_type]['first_name'])) AS first_name,
            $to_title(Yson::ConvertToString(person_data[original_type]['middle_name'])) AS middle_name,
            $to_title(Yson::ConvertToString(person_data[original_type]['last_name'])) AS last_name,
            Yson::ConvertToString(person_data[original_type]['longname']) AS longname,
            Yson::ConvertToString(person_data[original_type]['name']) AS name,
            IF(Yson::IsString(person_data[original_type]['inn']),
               Yson::ConvertToString(person_data[original_type]['inn']), NULL) AS inn
        FROM `//home/cloud-dwh/data/prod/ods/billing/person_data/person_data`
    ) AS t
ORDER BY billing_account_id
;
