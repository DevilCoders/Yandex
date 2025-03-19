PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;
IMPORT `helpers` SYMBOLS $get_md5;

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
            when 'description' then 'crm_call_description' 
            when 'id' then 'crm_call_id' 
            when 'name' then 'crm_call_name' 
            when 'status' then 'crm_call_status'
            when 'team_id' then 'crm_team_id'
            when 'team_set_id' then 'crm_team_set_id'
            when 'acl_team_set_id' then 'acl_crm_team_set_id'
            else `field_name`
            end )                                                                AS field_name,
        $toString(`id`)                                                          AS id,
        $toString(`parent_id`)                                                   AS parent_id,
        if(`field_name` in ('from_phone','to_phone'), 1, 0)                      AS is_pii
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT 
    if(is_pii=1, $get_md5(after_value_string),  after_value_string)             AS after_value_string,
    if(is_pii=1, $get_md5(after_value_text),    after_value_text)               AS after_value_text,
    if(is_pii=1, $get_md5(before_value_string), before_value_string)            AS before_value_string,
    if(is_pii=1, $get_md5(before_value_text),   before_value_text)              AS before_value_text,
    created_by                                                                  AS created_by,
    data_type                                                                   AS data_type,
    date_created_ts                                                             AS date_created_ts,
    date_created_dttm_local                                                     AS date_created_dttm_local,
    date_updated_ts                                                             AS date_updated_ts,
    date_updated_dttm_local                                                     AS date_updated_dttm_local,
    event_id                                                                    AS event_id,
    field_name                                                                  AS field_name,
    id                                                                          AS id,
    parent_id                                                                   AS parent_id 
FROM $result
;

/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT 
    after_value_string                                                          AS after_value_string,
    after_value_text                                                            AS after_value_text,
    before_value_string                                                         AS before_value_string,
    before_value_text                                                           AS before_value_text,
    created_by                                                                  AS created_by,
    data_type                                                                   AS data_type,
    date_created_ts                                                             AS date_created_ts,
    date_created_dttm_local                                                     AS date_created_dttm_local,
    date_updated_ts                                                             AS date_updated_ts,
    date_updated_dttm_local                                                     AS date_updated_dttm_local,
    event_id                                                                    AS event_id,
    field_name                                                                  AS field_name,
    id                                                                          AS id,
    parent_id                                                                   AS parent_id
FROM $result
where is_pii=1
;
