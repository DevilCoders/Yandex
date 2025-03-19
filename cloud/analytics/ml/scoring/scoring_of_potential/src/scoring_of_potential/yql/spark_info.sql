USE hahn;

$spark_all = '{spark_path}';
$result_table = '{result_table}';
PRAGMA File('libcrypta_identifier_udf.so', 'yt://hahn/home/crypta/public/udfs/libcrypta_identifier_udf.so');
PRAGMA Udf('libcrypta_identifier_udf.so');



$parseFloat = ($str) -> {{
    $listStr = String::SplitToList($str, ',');
    $int = $listStr[0];
    $rem = $listStr[1] ?? '0';
    RETURN CAST($int AS Int64) + CAST($rem AS Int64)/Math::Pow(10, LENGTH($rem));
}};

$parsePhones = ($phone) -> {{
    $concat_phone = nvl($phone.code, '') || nvl($phone.number, '');
    $normalized_phone = Identifiers::Phone($concat_phone).Normalize; 
    RETURN CAST($normalized_phone AS String)
}};

$selected_fields = (
    SELECT spark_id,
           guid,
           is_firm,
           name,
           $parseFloat(company_size.revenue) AS company_size_revenue,
           company_size.description AS company_size_description,
           company_size.actual_date AS company_size_actual_date,
           company_type,
           date_first_reg,
           inn,
           main_okved2_code,
           workers_range,
           population_date,
           legal_city,
           legal_region,
           okato.region_name AS okato_region_name,
           finance_and_taxes_fts,
           domain,
           email,
           egrul_likvidation,
           main_okved2_name,
           CAST(ListMap(phone_list, $parsePhones) AS List<String>) AS phones_list,
           String::SplitToList(workers_range, ' ') AS workers_range_list
    FROM $spark_all AS spark
);


INSERT INTO $result_table WITH TRUNCATE 
SELECT 
    selected_fields.*, 
    CAST(workers_range_list[0] AS Int32) as min_workers_range, 
    CAST(workers_range_list[ListLength(workers_range_list)-1] AS Int32) as max_workers_range
FROM $selected_fields AS selected_fields

