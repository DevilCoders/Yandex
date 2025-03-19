USE hahn;

DEFINE SUBQUERY $sender_delivery($path) AS
SELECT * 
FROM (
        SELECT
                recepient AS email,
                campaign_id,
                campaign_type,
                `message-id` AS message_id,
                channel,
                tags,
                status,
                letter_id,
                'send' AS event,
                CAST(unixtime AS Uint64) * 1000 AS unixtime,
                letter_code,
                NULL AS link_url,
                NULL AS title,
                NULL AS message_type,
                'sender' AS source
        FROM
                $path()
        WHERE
                `message-id` IS NOT NULL
                AND for_testing = 'False'
                AND account = 'yandex.cloud'
                AND status='0'
        )
FLATTEN LIST BY  (ListMap(Yson::ConvertToList(Yson::ParseJson(tags)), 
                        ($x) -> { RETURN Yson::ConvertToString($x); } ) as tags)
END DEFINE;

EXPORT $sender_delivery;