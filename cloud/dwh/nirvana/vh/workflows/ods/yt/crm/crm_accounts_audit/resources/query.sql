PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};

$toString = ($data) -> (CAST($data AS String));
$decode_utf8 = ($data) -> (nvl(cast(String::Base64StrictDecode($data) as Utf8),$data));

$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));
$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));

$result = (
    SELECT
        $toString(if(`field_name` in ('description'),$decode_utf8(`after_value_string`),`after_value_string`))                                                    
                                                                                AS after_value_string,
        $toString(if(`field_name` in ('description'),$decode_utf8(`after_value_text`),`after_value_text`))                                                    
                                                                                AS after_value_text,
        $toString(if(`field_name` in ('description'),$decode_utf8(`before_value_string`),`before_value_string`))                                                    
                                                                                AS before_value_string,
        $toString(if(`field_name` in ('description'),$decode_utf8(`before_value_text`),`before_value_text`))                                                    
                                                                                AS before_value_text,
        $toString(`created_by`)                                                 AS created_by,
        $toString(`data_type`)                                                  AS data_type,
        $get_timestamp(`date_created`)                                          AS date_created_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_created`))                  AS date_created_dttm_local,
        $get_timestamp(`date_updated`)                                          AS date_updated_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_updated`))                  AS date_updated_dttm_local,
        $toString(`event_id`)                                                   AS event_id,
        $toString(case `field_name` 
            when 'description' then 'crm_account_description'
            when 'id' then 'crm_account_id'
            when 'role' then 'crm_account_name'
            when 'acl_team_set_id' then 'acl_crm_team_set_id'
            when 'balance_id' then 'crm_balance_id'
            when 'campaign_id' then 'crm_campaign_id'
            when 'currency_id' then 'crm_currency_id'
            when 'segment_id' then 'crm_segment_id'
            when 'team_id' then 'crm_team_id'
            when 'team_set_id' then 'crm_team_set_id'
            else `field_name`
            end )                                                                AS field_name,
        $toString(`id`)                                                          AS id,
        $toString(`parent_id`)                                                   AS parent_id
    FROM $src_table
);


/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * 
FROM $result
 where field_name not in ( 
     'crm_account_description','billing_address_country','billing_address_postalcode','billing_address_state','billing_address_street','facebook','first_name','googleplus','last_name','crm_account_name','phone_alternate','phone_fax','phone_office','shipping_address_country','shipping_address_postalcode','shipping_address_state','shipping_address_street','twitte'
 )
ORDER BY `field_name`,`date_updated_ts`;

/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT *
 FROM $result
 where field_name in ( 
     'crm_account_description','billing_address_country','billing_address_postalcode','billing_address_state','billing_address_street','facebook','first_name','googleplus','last_name','crm_account_name','phone_alternate','phone_fax','phone_office','shipping_address_country','shipping_address_postalcode','shipping_address_state','shipping_address_street','twitte'
 )
ORDER BY `field_name`,`date_updated_ts`;