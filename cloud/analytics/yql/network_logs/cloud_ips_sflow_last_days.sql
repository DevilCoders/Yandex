USE hahn;

PRAGMA yson.DisableStrict;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;




DEFINE ACTION  $collect_ips_by_date($date, $result_table) AS
    $billing_ips = (
        SELECT
            cloud_id,
            folder_id,
            Yson::LookupString(tags, 'instance_id') AS instance_id,
            Yson::LookupString(tags, 'address')     AS address,
            Yson::LookupBool  (tags, 'ephemeral')   AS ephemeral,
            Yson::LookupUint64(usage, 'start') 
                - (CAST(Yson::LookupString(usage, 'quantity') AS UInt64) ?? 0UL) AS start,
            Yson::LookupUint64(usage, 'finish')     AS finish
          FROM RANGE('logs/yc-billing-metrics/1d', $date, $date)
          WHERE schema = 'vpc.address.v1'
    );
    
    $sflows = (
        SELECT  src_ip,
                dst_ip,
                unixtime
        FROM RANGE('logs/antifraud-overlay-sflows/1d', $date, $date)
    );
    
    INSERT INTO $result_table  
    SELECT *
    FROM $billing_ips AS billing_ips
    LEFT JOIN $sflows AS sflows
        ON billing_ips.address = sflows.src_ip
    WHERE unixtime BETWEEN start AND finish;
END DEFINE;

DEFINE ACTION  $collect_ips_last_days() AS
    $result_table = '//home/cloud_analytics/import/network-logs/sflows/cloud_ips_sflow_last_days';
    $dates =  CAST(
                CAST(ListFromRange(
                      Unwrap(CAST(CurrentUtcDate()- Interval('P20D') AS Uint32)), 
                      UnWrap(CAST(CurrentUtcDate() AS Uint32))) 
                AS List<Date>) 
              AS List<String>);
    
    DROP TABLE $result_table;
    COMMIT;

    EVALUATE FOR $date IN $dates DO $collect_ips_by_date($date, $result_table);
END DEFINE;

EXPORT $collect_ips_last_days