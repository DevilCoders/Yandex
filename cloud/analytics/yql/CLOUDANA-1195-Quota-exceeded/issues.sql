use hahn;

DEFINE SUBQUERY $issues() AS
    $type_quota_increase = "5dd52152a2b79e001d6d4ea8";
    
/* заведенные тикеты */
    $issues_help_table = (
        SELECT
            cloud_id,
            Yson::ConvertToString(components_array) AS components_array,
            key,
            created_datetime,
            resolved_datetime
        FROM(
            SELECT 
                Yson::LookupString(customFields, "cloudId") AS cloud_id,
                Yson::ConvertToList(components) AS components_array,
                key,
                created AS created_datetime,
                resolved AS resolved_datetime
            FROM `//home/cloud_analytics/import/startrek/support/issues` AS issues
            WHERE ListHas(Yson::ConvertToStringList(components), $type_quota_increase)
            AND Yson::Contains(customFields, "cloudId")
        ) AS t
        FLATTEN BY components_array
    );

    SELECT 
        issues.cloud_id AS cloud_id,
        issues.key AS key,
        issues.created_datetime AS created_datetime,
        issues.resolved_datetime AS resolved_datetime,
        AGGREGATE_LIST(components.name) AS components_name
    FROM $issues_help_table AS issues
    INNER JOIN `//home/cloud_analytics/import/startrek/support/components` AS components 
            ON components.id=issues.components_array
    WHERE issues.components_array != $type_quota_increase
    GROUP BY 
        issues.cloud_id,
        issues.key,
        issues.created_datetime,
        issues.resolved_datetime
END DEFINE;

EXPORT $issues;