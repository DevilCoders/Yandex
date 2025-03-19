use hahn;

$to_string = ($yson)->{RETURN Yson::ConvertToString($yson)};
$get_data = ($yson)->{RETURN Yson::Lookup($yson,'data')};

$get_yandex_email = ($yson) -> {RETURN $to_string(Yson::Lookup(Yson::Lookup($get_data($yson), 'answer_non_profile_email_10315809'), 'value'))};
$get_other_email = ($yson) -> {RETURN $to_string(Yson::Lookup(Yson::Lookup($get_data($yson), 'email'), 'value'))};
                                $get_nick = ($yson) -> {RETURN $to_string(Yson::Lookup(Yson::Lookup($get_data($yson), 'answer_short_text_10315812'),'value'))
                                };                                
$get_nick_from_email = ($str) -> {RETURN String::SplitToList($str, '@')[0] };                           

$to_datetime = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };

DEFINE subquery                             
$forma_fills() AS (
    SELECT
        yandexuid,
        puid,
        yandex_email,
        other_email,
        nick,
        MIN(created) AS created
    FROM(
        SELECT 
            yandexuid AS yandexuid,
            uid AS puid,
            $to_datetime(created) AS created,
            $get_yandex_email(answer) AS yandex_email,
            $get_other_email(answer) AS other_email,
            CASE
                WHEN Yson::Contains($get_data(answer), 'answer_short_text_10315812')
                THEN $get_nick(answer)
                ELSE $get_nick_from_email($get_yandex_email(answer))
            END AS nick
        FROM `//home/forms/answers/forms_ext/production/10024615/data`
        WHERE $get_yandex_email(answer) NOT LIKE '%mail.ru%'
            AND $get_yandex_email(answer) NOT LIKE '%gmail.com%'
        )
    GROUP BY 
        yandexuid,
        puid,
        yandex_email,
        other_email,
        nick
    );
END DEFINE;

EXPORT $forma_fills;