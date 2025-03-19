USE hahn;

$result_table = '//home/cloud_analytics/tmp/antifraud/sdn_features';
$compute_features = "//home/cloud_analytics/tmp/antifraud/compute_features";

$structs = (SELECT
                    AsStruct(`bm.cloud_id` AS cloud_id,
                            `bm.resource_id` AS resource_id,
                            `bm.folder_id` AS folder_id,
                            billing_account_id AS billing_account_id,
                            account_name AS account_name,
                            `bm.schema` AS schema),
                    Yson:: ConvertTo(`bm.usage`, Struct<finish:Uint64,
                                    quantity:String,
                                    start:Uint64,
                                    type:String,
                                    'unit':String>),
                    RenameMembers(Yson::ConvertTo(`bm.tags`, Struct<direction:String,
                                                                     type:String>), [('type', 'connection_type')])
FROM hahn.`home/cloud_analytics/tmp/antifraud/all_1h_after_trial`
WHERE `bm.schema` == 'sdn.traffic.v1');

$flatten = (SELECT * 
            from $structs
            FLATTEN COLUMNS);
            
$sdn_features = (SELECT billing_account_id, cloud_id, account_name,
                        SUM(if(direction='ingress', cast(quantity as Uint64)/8, 0)) as ingress_bytes,
                        SUM(if(direction='egress',  cast(quantity as Uint64)/8, 0)) as egress_bytes

                FROM $flatten
                GROUP BY billing_account_id, cloud_id, account_name);


insert into  $result_table  with truncate          
select * 
from $compute_features AS cf
full join $sdn_features  AS sdn
        on cf.cloud_id            = sdn.cloud_id
        and cf.billing_account_id = sdn.billing_account_id
        and cf.account_name       = sdn.account_name;


