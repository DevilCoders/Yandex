USE hahn;

DEFINE SUBQUERY $sender_clicks($path) AS
SELECT * 
FROM (
        SELECT
                email,
                campaign_id,
                NULL AS campaign_type,
                message_id,
                NULL AS channel,
                tags,
                NULL AS status,
                letter_id,
                event,
                CAST(unixtime AS Uint64) AS unixtime,
                NULL AS letter_code,
                link_url,
                NULL AS title,
                NULL AS message_type,
                'sender' AS source
        FROM
                $path()
        WHERE
                message_id IS NOT NULL
                AND account = 'yandex.cloud'
                and for_testing = 'False'
        )
FLATTEN LIST BY  (ListMap(Yson::ConvertToList(Yson::ParseJson(tags)), 
                        ($x) -> { RETURN Yson::ConvertToString($x); } ) as tags)
END DEFINE;

EXPORT $sender_clicks;