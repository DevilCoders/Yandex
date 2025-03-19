#!/usr/bin/env python3
"""This module contains constants."""

MINUTE = 60
HOUR = 60 * MINUTE
DAY = 24 * HOUR
WEEKENDS = (6, 7,)
DEFAULT_TIMEOUT = 10
DEFAULT_LOG_BACKUP_COUNT = 1
DEVELOPERS = (92504110, 450576562, 114035027, 229622190, 519934888)
KNOWN_ERRORS = []
DEFAULT_MAX_LOGFILE_SIZE = 2 * 1024 ** 2
RESPS_BASE_URL = 'https://resps-api.cloud.yandex.net'
STAFF_BASE_URL = 'https://staff-api.yandex-team.ru'
STARTREK_BASE_URL = 'https://st-api.yandex-team.ru'
SK_BASE_URL = 'http://127.0.0.1'
ABC2_BASE_URL = 'https://abc-back.yandex-team.ru'
BLACKBOX_BASE_URL = 'https://blackbox.yandex-team.ru'
STARTREK_BASE_FILTER = 'Queue: CLOUDSUPPORT and Updated: >= now() - 1d'
BASE_CLOUDSUPPORT_FILTER = 'Queue: CLOUDSUPPORT and Status: Open "Sort By": SLA ASC'
BASE_YCLOUD_FILTER = 'Queue: YCLOUD and Assignee: empty() and Status: New'
BASE_OPS_FILTER = 'Queue: CLOUDOPS and Tags: support and Resolution: empty()'
BASE_CLOUDABUSE_FILTER = 'Queue: CLOUDABUSE and Status: Open and Components: "РКН"'
BASE_SLA_FAILED_URL = 'https://st.yandex-team.ru/CLOUDSUPPORT/order:updated:false/filter?resolution=empty()&status=open&sla.violationStatus=FAIL_CONDITIONS_VIOLATED'
BASE_SLA_WARNING_URL = 'https://st.yandex-team.ru/CLOUDSUPPORT/order:updated:false/filter?resolution=empty()&status=open&sla.violationStatus=WARN_CONDITIONS_VIOLATED' 
BASE_REOPEN_URL = 'https://st.yandex-team.ru/CLOUDSUPPORT/order:updated:true/filter?resolution=empty()&status=1&tags=reopen'
BASE_CHAT_URL =  'https://app.chatra.io/conversations'

SK_ISSUES_ENDPOINT = 'https://st-api.yandex-team.ru/v2/issues/_search'

CLOUD_DEPARTMENT = u'yandex_exp_9053'
SUPPORT_DEPARTMENT = u'yandex_exp_9053_4245_dep41451'
DEFAULT_SCHEDULE_ID = 30470

BOOLEAN_KEYS = ('is_allowed', 'is_admin', 'cloudsupport',
                'cloudabuse', 'ycloud', 'do_not_disturb',
                'premium_support', 'business_support',
                'standard_support', 'is_duty',
                'ignore_work_time', 'is_support', 'cloudops')

USERAGENT = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_6) ' + \
            'AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 ' + \
            'YaBrowser/18.1.1.841 Yowser/2.5 Safari/537.36'

DUTY_STAFF = ('the-nans', 'sinkvey', 'hadesrage', 'dymskovmihail', 'nooss', 'd-penzov', 'cameda', 'eremin-sv', 'teminalk0', 'snowsage', 'v-pavlyushin', 'hercules', 'krestunas')
DUTY_DAY1_STAFF = ('cameda', 'krestunas', 'eremin-sv', 'sinkvey')
DUTY_DAY2_STAFF = ('v-pavlyushin', 'hercules', 'nooss')
DUTY_NIGHT1_STAFF = ('teminalk0', 'd-penzov')
DUTY_NIGHT2_STAFF = ('dymskovmihail', 'snowsage')
# SQL REQUEST CONSTANTS
sql_constants = {
    'MESSAGE_TO_SV1': "select su.login, u.telegram_id from s_keeper_support_unit su left join users u on "
                      "su.login = u.staff_login where su.login like",
    'MESSAGE_TO_SV': """
        select su.login, u.telegram_id from s_keeper_support_unit su left join users u on su.login = u.staff_login 
        where su.rank < 3 and u.telegram_id IS NOT NULL;    
        """,
    'SET_DND': "update s_keeper_support_unit set is_absent=1 where login=",
    'ST_ISSUES': """
    select
        s_keeper_queue_filter.name, s_keeper_queue_filter.ts_open, s_keeper_queue_filter.query_type,
        sksu.login from s_keeper_queue_filter left join
        s_keeper_queue_filter_assignees skqfa on s_keeper_queue_filter.q_id = skqfa.queue_filter_id
        left join s_keeper_support_unit sksu on skqfa.support_unit_id = sksu.u_id
        where login like """,
    'SK_TO_TG_ID' : """
    select sksu.u_id from s_keeper_support_unit sksu left join users u on sksu.login = u.staff_login where telegram_id = %s;
    """,
'GET_ASSIGNMENT_CHANGES' :  """select * from
            (select sk3.last_supervisor, sk1.queue_filter_id as new_qfid, sk1.support_unit_id as new_suid,
            sk2.support_unit_id as old_suid, sk2.queue_filter_id as old_qfid,
                    sksu.login as login
            from s_keeper_queue_filter_assignees as sk1
            left join s_keeper_queue_state as sk2
            on sk1.queue_filter_id = sk2.queue_filter_id and sk1.support_unit_id = sk2.support_unit_id
            left join s_keeper_support_unit sksu on sk1.support_unit_id = sksu.u_id
            left join s_keeper_queue_filter sk3 on sk3.q_id = sk1.queue_filter_id
            union all
            select sk3.last_supervisor, sk1.queue_filter_id as new_qfid, sk1.support_unit_id as new_suid,
            sk2.support_unit_id as old_suid, sk2.queue_filter_id as old_qfid,
                   sksu.login as login
            from s_keeper_queue_filter_assignees as sk1
                right join s_keeper_queue_filter sk3 on sk3.q_id = sk1.queue_filter_id
            right join s_keeper_queue_state as sk2
            on sk1.queue_filter_id = sk2.queue_filter_id and sk1.support_unit_id = sk2.support_unit_id
            right join s_keeper_support_unit sksu on sk2.support_unit_id = sksu.u_id) as c
            where old_qfid is null and new_qfid is not null or old_qfid is not null and new_qfid is null;
        """,
'SET_NEW_TASKS_STATE' : """
        truncate s_keeper_queue_state;
        insert into s_keeper_queue_state (queue_filter_id, support_unit_id)
        select queue_filter_id, support_unit_id from s_keeper_queue_filter_assignees;
        """,
'GET_SUPERVISORS': """
        select login from s_keeper_support_unit where 'is_absent' = 0 and `rank` <=2;
        """,
'GET_PARTY_ADD': """
                    select sksu.login from s_keeper_queue_filter_assignees skqfa left join s_keeper_support_unit sksu
                    on skqfa.support_unit_id = sksu.u_id where queue_filter_id like "%s"
                    """,
    'GET_PARTY_RM': """
                    select sksu.login from s_keeper_queue_state skqfa left join s_keeper_support_unit sksu
                    on skqfa.support_unit_id = sksu.u_id where queue_filter_id like "%s"
                    """,
    'GET_QUEUE': """
                    select name, q_id, last_supervisor from s_keeper_queue_filter where q_id like "%s";
            """,
    'GET_TARGET_USER_ADD': """
                    select sksu.login, sksu.u_id from s_keeper_queue_filter_assignees skqfa left join s_keeper_support_unit sksu
                    on skqfa.support_unit_id = sksu.u_id
                    where skqfa.support_unit_id like "%s"
                    """,
    "GET_TARGET_USER_RM": """
                    select sksu.login, sksu.u_id from s_keeper_queue_state skqfa left join s_keeper_support_unit sksu
                    on skqfa.support_unit_id = sksu.u_id
                    where skqfa.support_unit_id like "%s"
                    """,
    "PENDING_QUEST_INVITES": """
      select * from
    (select sqf.q_id, sqf.name, sqf.crew_warning_limit, count(skqfa.support_unit_id) as party from s_keeper_queue_filter sqf
        left join s_keeper_queue_filter_assignees skqfa on sqf.q_id = skqfa.queue_filter_id
    group by sqf.q_id) as c where c.party < c.crew_warning_limit ;
    """,
    "ACTUAL_QUESTS": """
    select * from(
    select sqf.q_id, sqf.name, sqf.crew_warning_limit, count(skqfa.support_unit_id) as party from s_keeper_queue_filter sqf
    left join s_keeper_queue_state skqfa on sqf.q_id = skqfa.queue_filter_id
    group by sqf.q_id order by party desc) as c where c.party < c.crew_warning_limit ;
    """,
    "SELFASSIGN1": """
    insert into s_keeper_queue_state (queue_filter_id, support_unit_id) values ("%s", "%s");
    """,
    "SELFASSIGN2": """
    insert into s_keeper_queue_filter_assignees (queue_filter_id, support_unit_id) values ("%s", "%s");
    """,
    "SELFREMOVE1": """
                        delete from s_keeper_queue_state skqfa where skqfa.queue_filter_id like "%s" and skqfa.support_unit_id like "%s";
                        """,
    "SELFREMOVE2": """
            delete from s_keeper_queue_filter_assignees skqfa where skqfa.queue_filter_id like "%s" and skqfa.support_unit_id like "%s";
            """,
    "GET_USERS_QUESTS": """
    select * from s_keeper_queue_state skqs left join s_keeper_queue_filter skqf on skqs.queue_filter_id = skqf.q_id 
    where skqs.support_unit_id like "%s"    
    """
}

text_messages = {
    'PROPOSE_QUEST': 'Лорд этих земель %s направил тебя в %s. Берём квест?',
    'END_QUEST': 'Хватит истреблять тикеты в %s! Пора вернуться за наградой, отважный авантюрист!',
    'ACCEPT_QUEST': '',
    "TAKE_QUEST": "Расспросив по округе, вы решили избавить жителей этого края от напастей в очереди",
    "TAKE_QUEST_NOTE": "Выберите очередь. Цифра - это сколько народу там нужно. Если народу не нужно, очередь не видно",
    "DROP_QUEST": "Ты с победой возвращаешься из квеста в очередь",
    "LIST_QUESTS": "Ты вписан вот в эти очереди(если не хочешь следить за какой-то - нажми на неё):",
    "WHICH_QUEST": "Какой квест прекращаем?",
    "QUEST_ACCEPTED": "Бригаде приключенцев везде найдётся квест! - ты и твоя партия c энтузиазмом устремляетесь к выходу...",
    "QUEST_DECLINED": "Жители деревни не одобряют тебя. Что ж, рано или поздно найдётся новый герой.",
    "QUEST_DECLINED_NOTE": "Коллега %s отклонил квест в очередь %s",
    "QUEST_ACCEPTED_NOTE": "Коллега %s принял квест в очередь %s",
}
