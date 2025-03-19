use hahn;

$to_datetime = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };

DEFINE SUBQUERY
$cloud_created() AS (
    SELECT 
        cube1.puid AS puid,
        cube1.email AS email,
        cube2.billing_account_id AS billing_account_id,
        MIN($to_datetime(cube1.event_time)) AS cloud_created_time,
        MIN($to_datetime(cube2.event_time)) AS ba_created_time
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube` AS cube1 
    LEFT JOIN (SELECT 
                    puid,
                    MIN(event_time) AS event_time,
                    billing_account_id
                FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
                WHERE 
                    event ="ba_created"
                GROUP BY 
                    puid,
                    billing_account_id) AS cube2 ON cube1.puid=cube2.puid
    WHERE 
        cube1.event ="cloud_created"
    GROUP BY 
        cube1.puid, 
        cube1.email,
        cube2.billing_account_id
    );
END DEFINE;

EXPORT $cloud_created;