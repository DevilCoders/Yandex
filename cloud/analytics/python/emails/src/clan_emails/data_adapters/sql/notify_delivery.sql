USE hahn;
PRAGMA yson.DisableStrict;


DEFINE SUBQUERY $notify_delivery($path) AS 
$_notify = (
        SELECT
                fields AS nfields,
                Yson::ConvertToString(Yson::YPath(fields, '/req/id')) AS nreq_id,
                Yson::YPath(fields, '/extra') AS extra,
                '/mail/px.png?utm_id=' || 
                        Yson::ConvertToString(
                                Yson::YPath(fields, '/req/id')) AS url,
                qloud_component as source
        FROM
                $path()
        WHERE
                qloud_project = 'ya-cloud-front'
                AND qloud_application = 'console-tools'
                AND qloud_component = 'notify'
                AND qloud_environment = 'ext-prod'
                AND Yson::YPath(fields, '/extra/receiver') IS NOT NULL
);

SELECT
        Yson::ConvertToString(Yson::YPath(extra, '/receiver')) AS email,
        NULL AS campaign_id,
        NULL AS campaign_type,
        nreq_id AS message_id,
        NULL AS channel,
        NULL AS tags,
        NULL AS status,
        NULL AS letter_id,
        'send' AS event,
        Yson::ConvertToUint64(Yson::YPath(nfields, '/time')) AS unixtime,
        NULL AS letter_code,
        '/mail/px.png?utm_id=' || nreq_id AS link_url,
        Yson::ConvertToString(Yson::YPath(extra, '/data/eventTitle/en')) AS title,
        Yson::ConvertToString(Yson::YPath(extra, '/type')) AS message_type,
        source
FROM
        $_notify
WHERE 
        nreq_id IS NOT NULL

END DEFINE;

EXPORT $notify_delivery;