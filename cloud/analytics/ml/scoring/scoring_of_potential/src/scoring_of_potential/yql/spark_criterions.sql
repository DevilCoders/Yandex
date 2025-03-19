USE hahn;

$result_table = '{result_table}';
$spark = '{spark_path}';

PRAGMA File('libcrypta_identifier_udf.so', 'yt://hahn/home/crypta/public/udfs/libcrypta_identifier_udf.so');
PRAGMA Udf('libcrypta_identifier_udf.so');


DEFINE SUBQUERY $get_spark_attribute($column) AS
    SELECT DISTINCT  
        spark_id, 
        $column AS criterion,
        nvl(CAST(TableRow().$column AS String), '') AS value,
    FROM $spark
END DEFINE;



$spark_attributes = (
    SELECT spark_id, 
           criterion, 
           nvl(value.code, '') || nvl(value.number, '') AS value
    FROM (
        SELECT DISTINCT  
            spark_id, 
            'phone' AS criterion,
            phone_list AS value,
        FROM $spark
    )
    FLATTEN LIST BY value
    
    UNION ALL
    
    SELECT *
    FROM $get_spark_attribute('inn')
    
    UNION ALL
    
    SELECT *
    FROM $get_spark_attribute('name')
    
    UNION ALL
    
    SELECT *
    FROM $get_spark_attribute('email')
);

INSERT INTO $result_table WITH TRUNCATE
SELECT 
    spark_id,
    criterion,
    CASE criterion 
         WHEN 'phone' THEN Identifiers::Phone(value).Normalize
         WHEN 'email' THEN Identifiers::Email(value).Normalize
         ELSE value 
    END AS value
FROM $spark_attributes
WHERE value != ''