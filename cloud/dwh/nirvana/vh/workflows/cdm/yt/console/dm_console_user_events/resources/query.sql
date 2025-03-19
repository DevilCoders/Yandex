$src_folder = {{ param["source_folder_path"] -> quote() }};
$destination_table = {{ input1 -> table_quote() }};

INSERT INTO $destination_table
    WITH TRUNCATE
SELECT
    event_date,
    service,
    goals_reached,
    COUNT(hit_id)                           AS hits
FROM
    (SELECT
        TableName()                         AS event_date,
        NVL(hit_id, CAST('0' AS Uint64))    AS hit_id,
        goals_reached,
        String::SplitToList(Url::GetDomain(referer, UNWRAP(CAST(Url::GetDomainLevel(referer) AS Uint8))), '.')[0] AS service,
    FROM RANGE($src_folder, `2022-03-02`) AS br
    WHERE
        counter_id = 51465824
        AND ListHasItems(goals_reached)
        AND hit_id IS NOT NULL
        AND is_robot IS NULL
        AND referer NOT LIKE '%staging%')
FLATTEN LIST BY goals_reached
WHERE goals_reached IN (219279787, 219286519, 219286612, 219286534, 219286561, 219286597, 219286636, 219286663, 229825829, 229825834, 229825837)
GROUP BY
    event_date,
    service,
    goals_reached
ORDER BY
    event_date,
    service,
    goals_reached
