use hahn;
$sku = '//home/logfeller/logs/yc-billing-export-sku/1h';
$result_path = '//home/cloud_analytics/export/crm/sku';
DEFINE SUBQUERY $last_non_empty_table($path) AS
    $max_path = (
        SELECT MAX(Path) AS Path
        FROM FOLDER($path, 'row_count')
        WHERE Type = 'table'
            AND Yson::LookupInt64(Attributes, 'row_count') > 0
    );
    SELECT * FROM CONCAT($max_path);
END DEFINE;

$n_capture_ru = Re2::Capture(
    '.*{\\"ru\\": {\\"name\\": \\"(?P<runame>.*)\\"}, \\"en\\": {\\"name\\": \\"(?P<engname>.*)\\"}}'
);


$n_capture_en = Re2::Capture(
    '.*{\\"en\\": {\\"name\\": \\"(?P<engname>.*)\\"}, \\"ru\\": {\\"name\\": \\"(?P<runame>.*)\\"}}'
);

$name_capture_ru = ($str) -> {
    RETURN $n_capture_ru($str); -- apply callable to user_agent column
};

$name_capture_en = ($str) -> {
    RETURN $n_capture_en($str); -- apply callable to user_agent column
};

$pv_capture = Re2::Capture(
    '.*\\"rates\\": (?P<pricing_versions>.*?}])'
);

$pricing_versions_capture = ($str) -> {
    RETURN $pv_capture($str); -- apply callable to user_agent column
};


$script = @@
def unicode_byte_utf_and_other_magic_to_see_normal_russian_language(val):
    if val is None or val != val:
        return val
    return val.decode('unicode-escape').encode('utf-8').decode('utf-8')
@@;

$callable = Python3::unicode_byte_utf_and_other_magic_to_see_normal_russian_language(   
    Callable<(String?)->UTF8?>,
    $script                             -- use python script contents
);

$replace = Re2::Replace('\\"');

$result = SELECT 
    $callable(name_ru) as _name_ru,
    $callable(name_eng) as _name_eng,
    sku,
    id,
    $replace(_pricing_versions, "") as pricing_versions,
    pricing_unit,
    service_long_name,
    updated_at
    
FROM (
    SELECT 
        IF($name_capture_ru(languages).runame is null, $name_capture_en(languages).runame, $name_capture_ru(languages).runame) AS name_ru,
        IF($name_capture_ru(languages).engname is null, $name_capture_en(languages).engname, $name_capture_ru(languages).engname) as name_eng,
        name as sku,
        id,
        $pricing_versions_capture(Yson::ConvertToString(pricing_versions)).pricing_versions as _pricing_versions,
        pricing_unit,
        updated_at
    FROM (
        SELECT
            Yson::ConvertToString(translations) as languages,
            name,
            id,
            pricing_versions,
            pricing_unit,
            updated_at
        FROM $last_non_empty_table($sku)
    )
) as a
INNER JOIN (
    SELECT sku_id, service_long_name FROM `home/cloud_analytics/tmp/artkaz/sku_tags`
) as b
ON a.id == b.sku_id;

INSERT INTO $result_path WITH TRUNCATE
     SELECT * FROM $result
     ORDER BY updated_at DESC;