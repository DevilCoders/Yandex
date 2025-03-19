USE hahn;
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
                    Yson::ConvertTo(`bm.tags`, Struct<cores:Uint32,
                                                        memory:Uint64,
                                                        public_fips:Uint32,
                                                        platform_id:String,
                                                        sockets:Uint32>)
FROM hahn.`home/cloud_analytics/tmp/antifraud/all_1h_after_trial`
WHERE `bm.schema` == 'compute.vm.generic.v1');

$flatten = (SELECT * 
            from $structs
            FLATTEN COLUMNS);
            
INSERT INTO hahn.`home/cloud_analytics/tmp/antifraud/compute_features` WITH TRUNCATE 
SELECT billing_account_id, cloud_id, account_name,
       min(cores) as min_cores,
       max(cores) as max_cores,
       median(cores) as median_cores,

       min(memory) as min_memory,
       max(memory) as max_memory,
       median(memory) as median_memory,
  
       
       median(sockets) as median_sockets,
       median(public_fips) as median_public_fips,
       count(distinct resource_id) as vm_count

FROM $flatten
GROUP BY billing_account_id, cloud_id, account_name