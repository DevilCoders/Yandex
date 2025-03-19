USE hahn;
$utm_id = Re2::Capture('utm_id=(\\w*)');

DEFINE SUBQUERY $console_clicks($path) AS
SELECT
        NULL AS email,
        NULL AS campaign_id,
        NULL AS campaign_type,
        $utm_id(url)._1 AS message_id,
        NULL AS channel,
        NULL AS tags,
        CAST(
                Yson::ConvertToInt64(
                        Yson::YPath(_rest ['@fields'], '/res/statusCode')
                ) AS String
        ) AS status,
        NULL AS letter_id,
        'px' AS event,
        Yson::ConvertToUint64(Yson::YPath(_rest ['@fields'], '/time')) AS unixtime,
        NULL AS letter_code,
        Yson::ConvertToString(Yson::YPath(_rest ['@fields'], '/req/url')) AS link_url,
        NULL AS title,
        NULL AS message_type,
        'cloud-console' AS source
FROM
        $path()
WHERE
        url LIKE '%/mail/px.png%' 
        AND $utm_id(url)._1  IS NOT NULL
END DEFINE;

EXPORT $console_clicks;