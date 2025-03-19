use hahn;

pragma RegexUseRe2 = 'true';

declare $date as String;

$folder_capture = ($log) -> {
    $capture = Re2::Capture("'folderId': '(?P<folder>[a-z0-9]+)'");
    RETURN $capture($log); -- apply callable to user_agent column
};

$message_capture = ($log) -> {
    $capture = Re2::Capture("API request returned an error \\[QuotaExceeded\\]: (?P<error_msg>.*)");
    RETURN $capture($log); -- apply callable to user_agent column
};

$result_path = "//home/cloud_analytics/dwh/ods/compute/quota_exceeded_log/" || $date;
$lb_path =  "//home/logfeller/logs/yandexcloud-prod-log/1d/" || $date;

insert into $result_path with truncate
select
    l.event_dttm as event_dttm,
    l.request_id as request_id,
    l.message as message,
    l.folder_id as folder_id,
    cf.cloud_id as cloud_id,
    l.error_msg as error_msg,
    case
        when error_msg regexp "\\[QuotaExceeded\\] The balancer can have a maximum of \\d+ listeners\\." then "network-load-balancer-listener-count"
        when error_msg = "[QuotaExceeded] The limit on maximum amount of memory has exceeded." then "memory"
        when error_msg = "[QuotaExceeded] The limit on maximum number of GPU devices has exceeded." then "gpu"
        when error_msg = "[QuotaExceeded] The limit on maximum number of cores has exceeded." then "instance-cores"
        when error_msg = "[QuotaExceeded] The limit on maximum number of disks has exceeded." then "disk-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of external addresses has exceeded." then "external-address-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of external direct SMTP addresses has exceeded." then "external-smtp-direct-address-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of external static addresses has exceeded." then "external-static-address-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of images has exceeded." then "image-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of instances has exceeded." then "instance-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of instances in placement group with spread placement strategy has exceeded." then "spread-placement-group-instance-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of network load balancers has exceeded." then "network-load-balancer-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of networks has exceeded." then "network-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of placement groups has exceeded." then "placement-group-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of route tables has exceeded." then "route-table-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of snapshots has exceeded." then "snapshot-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of subnets has exceeded." then "subnet-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of target groups has exceeded." then "target-group-count"
        when error_msg = "[QuotaExceeded] The limit on total size of network-hdd disks has exceeded." then "network-hdd-total-disk-size"
        when error_msg = "[QuotaExceeded] The limit on total size of network-ssd disks has exceeded." then "network-ssd-total-disk-size"
        when error_msg = "[QuotaExceeded] The limit on total size of snapshots has exceeded." then "snapshot-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of host groups has exceeded." then "host-group-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of disk placement groups has exceeded." then "disk-placement-group-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of external QRator addresses has exceeded." then "external-qrator-address-count"
        when error_msg = "[QuotaExceeded] The limit on total size of network-ssd-nonreplicated disks has exceeded." then "network-ssd-nonreplicated-total-disk-size"
        when error_msg = "[QuotaExceeded] The limit on maximum number of security groups has exceeded." then "security-group-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of static routes has exceeded." then "static-route-count"
        when error_msg = "[QuotaExceeded] The limit on maximum number of network load balancers has exceeded." then "network-load-balancer-count"
        else null
    end as resource_name
from (
    SELECT
        a1.`TIMESTAMP` as event_dttm,
        a1.`REQUEST_ID` as request_id,
        a1.`MESSAGE` as message,
        $folder_capture(a2.`MESSAGE`).folder as folder_id,
        $message_capture(a1.`MESSAGE`).error_msg as error_msg,
        row_number() over w as rn
        -- a2.`MESSAGE` as message2
    FROM $lb_path as a1
    join $lb_path as a2
      on a1.`REQUEST_ID` = a2.`REQUEST_ID`
    WHERE a1.`MESSAGE` ILIKE '%API request returned an error \[QuotaExceeded\]%'
      and a2.`MESSAGE` like '%\'folderId\': %'
    window w as (partition by  a1.`REQUEST_ID` order by  a1.`TIMESTAMP` desc)
) as l
 left
 join `//home/cloud_analytics/import/iam/cloud_folders/1h/latest` as cf
   on cf.folder_id = l.folder_id
 left
 join `//home/cloud_analytics/import/iam/cloud_owners_history` as co
   on co.cloud_id = cf.cloud_id
where rn = 1
;
