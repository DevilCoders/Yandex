$src_folder = {{ param["src_table"] -> quote() }};
$destination_table = {{ input1 -> table_quote() }};

INSERT INTO $destination_table WITH TRUNCATE

SELECT
    user_id,
    event_type,
    event_date

FROM

(SELECT
        promo.event_date                       AS event_date,
        promo.user_id                          AS user_id,
        ASList(promo.event_type,
            auth.event_type,
            auth_enter.event_type,
            welcome.event_type,
            new_user_dl_visit.event_type,
            enter_show.event_type,
            open_or_activate.event_type,
            old_user_dl_visit.event_type)      AS event_type
FROM
    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'promo'                            AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND ListHas(goals_reached, 231884342) --показ промо DL
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
    ) AS promo

LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'auth'                             AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 51465824
            AND ListHas(goals_reached, 167148289) --посещение auth
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
            AND parsed_params_key2[0] = 'yc.oauth.datalens' --пришли на auth с datalens
    ) AS auth

ON promo.user_id = auth.user_id AND promo.event_date = auth.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'auth_enter'                       AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 51465824
            AND ListHas(goals_reached, 219279349) --клик на "Войти" на auth
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
            AND parsed_params_key2[0] = 'yc.oauth.datalens' --пришли на auth с datalens
    ) AS auth_enter

ON auth.user_id = auth_enter.user_id AND auth.event_date = auth_enter.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'welcome'                          AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND ListHas(goals_reached, 231888290) --показ "Добро пожаловать в DataLens"
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
    ) AS welcome

ON auth_enter.user_id = welcome.user_id AND auth_enter.event_date = welcome.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'new_user_dl_visit'                AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND url LIKE '%skipPromo=true%'
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits IS NULL --не просмотры страницы
            AND is_robot IS NULL
    ) AS new_user_dl_visit

ON welcome.user_id = new_user_dl_visit.user_id AND welcome.event_date = new_user_dl_visit.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'enter_show'                       AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND ListHas(goals_reached, 231888254) --посещение "Войти"
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
    ) AS enter_show

ON promo.user_id = enter_show.user_id AND promo.event_date = enter_show.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'open_or_activate'                 AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND (ListHas(goals_reached, 231888259) OR ListHas(goals_reached, 231888276))--клик на "Открыть DataLens" или создание нового DataLens
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits = true --не просмотры страницы
            AND is_robot IS NULL
    ) AS open_or_activate

ON enter_show.user_id = open_or_activate.user_id AND enter_show.event_date = open_or_activate.event_date
LEFT JOIN

    (
        SELECT DISTINCT
            TableName()                        AS event_date,
            user_id,
            'old_user_dl_visit'                AS event_type
        FROM RANGE($src_folder, `2022-03-22`)
        WHERE
            counter_id = 50514217
            AND url LIKE '%skipPromo=true%'
            AND refresh IS NULL --не перезагрузки страницы
            AND dont_count_hits IS NULL --не просмотры страницы
            AND is_robot IS NULL
    ) AS old_user_dl_visit

ON open_or_activate.user_id = old_user_dl_visit.user_id AND open_or_activate.event_date = old_user_dl_visit.event_date)

FLATTEN BY
    event_type
WHERE
    event_type IS NOT NULL
ORDER BY
    event_date,
    event_type
;
