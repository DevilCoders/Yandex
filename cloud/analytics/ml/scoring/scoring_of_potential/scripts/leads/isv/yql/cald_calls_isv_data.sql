USE  hahn;
-- PRAGMA yt.UseNativeYtTypes;
PRAGMA File('libcrypta_identifier_udf.so', 'yt://hahn/home/crypta/public/udfs/libcrypta_identifier_udf.so');
PRAGMA Udf('libcrypta_identifier_udf.so');
$result_table = '//home/cloud_analytics/scoring_of_potential/leads/isv/isv_cold_calls';

$isv_companies = (
    SELECT
        CAST(CAST(`inn` AS Int64) AS String) as inn, 
        Yson::ConvertToString(`company_name`) as company_name,
        Yson::ConvertToStringList(`phone_number`) as phone_number,
        Yson::ConvertToString(`domain`) as domain,
        Yson::ConvertToString(`main_url`) as main_url
    FROM hahn.`home/cloud_analytics/scoring_of_potential/leads/isv_companies3`
    -- FLATTEN LIST BY phone_number
);


$flattened  = (
    SELECT *
    FROM (
        SELECT isv_companies.*, IF(ListLength(phone_number)=0, [NULL], phone_number) as phone_numbers WITHOUT phone_number
        FROM $isv_companies as isv_companies
    )
    FLATTEN LIST BY phone_numbers
);


$domains = (
    SELECT  inn, 
        company_name,   
        ListUniq([
            Url::NormalizeWithDefaultHttpScheme(Url::CutWWW(Url::CutScheme(Url::NormalizeWithDefaultHttpScheme(domain)))), 
            Url::NormalizeWithDefaultHttpScheme(Url::CutWWW(Url::CutScheme(Url::NormalizeWithDefaultHttpScheme(main_url))))
        ]) as domains, 
        Identifiers::Phone(phone_numbers).Normalize as phone_number
FROM $flattened
);


$cleaned_phones_and_domains = (
    SELECT  
        inn,
        AGG_LIST_DISTINCT(domains) AS domains,
        AGG_LIST_DISTINCT(phone_number) AS phone_number
    FROM (
        SELECT *
        FROM $domains
        FLATTEN LIST BY domains
    ) as domains
    -- WHERE domains IS NOT NULL
    GROUP BY inn
);


$onboarding_leads = '//home/cloud_analytics/scoring_of_potential/spark_info';

INSERT INTO $result_table WITH TRUNCATE 
SELECT  DISTINCT 
    onboarding_leads.name as name,
    onboarding_leads.company_size_revenue as revenue,
    onboarding_leads.workers_range as workers_range,
    onboarding_leads.email as email,
    cleaned_phones_and_domains.inn as inn,
    ListSortDesc(ListUniq(ListFlatten([cleaned_phones_and_domains.phone_number, phones_list]))) as phones,
    cleaned_phones_and_domains.domains as domains
FROM  $cleaned_phones_and_domains as cleaned_phones_and_domains
LEFT JOIN $onboarding_leads as onboarding_leads
ON cleaned_phones_and_domains.inn = CAST(onboarding_leads.inn as String)
