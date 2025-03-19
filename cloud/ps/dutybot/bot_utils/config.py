import logging
import os.path
import sys

import environ
import requests
import yaml

CFG_PATH = "config.yaml"


def get_current_az():
    r = requests.get("http://169.254.169.254/latest/meta-data/placement/availability-zone")
    r.raise_for_status()
    return r.text


TEST_CONFIG = {
    'database': {
        'name': '',
        'user': '',
        'pass': '',
        'host': '',
        'port': '',
    },
    'telegram': {
        'prod': '',
        'preprod': '',
        'debug': '',
    },
    'yandex_messenger': {
        'prod': '',
    },
    'auth': {
        'staff': '',
        'st': '',
    },
    'other': {
        'useragent': '',
        'tracker_endpoint': '',
        'staff_endpoint': '',
        'department': '',
        'resps_endpoint': '',
    },
    'chats': [],
}
IS_CODE_BEING_TESTED = 'pytest' in sys.modules


class Config:
    env = environ.Env()

    # The solution isn't an elegant one, however, it unblocks the testing
    # Refactoring of that class is highly advised
    if IS_CODE_BEING_TESTED:
        cfg = TEST_CONFIG
    else:
        if not os.path.isfile(CFG_PATH):
            logging.fatal("[config] Config file was not found!")
            raise FileNotFoundError()

        with open(CFG_PATH, "r") as cfg:
            cfg = yaml.safe_load(cfg)

    # Database
    db_name = cfg['database']['name']
    db_user = cfg['database']['user']
    db_pass = cfg['database']['pass']
    db_host = cfg['database']['host']
    db_port = cfg['database']['port']

    # Telegram
    tg_prod = cfg['telegram']['prod']
    tg_preprod = cfg['telegram']['preprod']
    tg_debug = cfg['telegram']['debug']

    # Yandex.Messenger
    yandex_messenger_prod = cfg['yandex_messenger']['prod']

    # Yandex services oAuth-tokens
    ya_staff = cfg['auth']['staff']
    ya_tracker = cfg['auth']['st']

    # Other
    useragent = cfg['other']['useragent']
    tracker_endpoint = cfg['other']['tracker_endpoint']
    staff_endpoint = cfg['other']['staff_endpoint']
    cloud_department = cfg['other']['department']
    resps_endpoint = cfg['other']['resps_endpoint']
    priority_types_startrek = {
        "1": "незначительный",
        "2": "нормальный",
        "3": "незначительный",
        "4": "критический",
        "5": "незначительный",
    }

    # Chats parser
    well_known_chats = cfg["chats"]

    # Fake teams list
    vpc_text = [
        "Команда VPC перешла к [раздельным дежурствам](https://clubs.at.yandex-team.ru/ycp/3154)\n",
        "— `/duty vpc-api` — публичный и приватный API, Terraform, квоты и лимиты, ошибки типа Permission Denied от VPC,",
        "— `/duty overlay` — связность внутри Облака, доступ до виртуальных машин, работа Security Groups и Static Routes, "
        "HBF, e2e-виртуалки «пермнеты» и «мисиксы», network-flow-collector и биллинг трафика, "
        "сетевые сервисы для инициализации ВМ: DHCP и Metadata.",
        "— `/duty cgw` — внешняя связность, Cloud Interconnect, NAT в Интернет и L3-балансировщик,",
        "— `/duty netinfra` — физическая («железная») сеть, стык Облака с внешним миром, проблемы с конкретным провайдером и куратором,"
        "— `/duty vpc-pm` — планирование фич и другие менеджерские вопросы.",
    ]
    fake_teams = {
        "vpc": vpc_text,
        "vpc-all": vpc_text,
        "ycp": [
            "Ответы на вопросы про утилиту `ycp` ищите на [вики](https://wiki.yandex-team.ru/cloud/devel/platform-team/dev/ycp).",
            "Если ответа не нашлось, напишите в рабочее время в [чат поддержки API self-service](https://t.me/joinchat/P0uloEwF29M81BfJukEO1g).",
            "Если долго нет ответа, призывайте там [skipor_yandex](https://t.me/skipor_yandex).",
        ],
        "ui": [
            "Есть следующие команды UI:\n",
            "— `/duty console` — консоль, нотифай",
            "— `/duty backoffice` — бэкофис, внешний сайт, портал, статусборд, мобильное приложение.",
        ],
        "datalens": [
            "Есть следующие команды DataLens:\n",
            "— `/duty datalens-front` — все про UI, начало работы и инициализация",
            "— `/duty datalens-back` — запросы в источники, формулы, материализация, биллинг",
            "— `/duty datalens-product` — продуктовые вопросы, консультации, уточнения документации, работа с партнерами",
        ],
        "compute": [
            "Команда compute перешла к раздельным дежурствам.",
            "Если у вас вопрос вида:",
            "— я хочу новую фичу или узнать сроки по давно ожидаемой,",
            "— виртуальной машине поплохело,",
            "— операция висит слишком долго,",
            "— возникла ошибка `ResourceExhausted` или `QuotaExceeded`,",
            "то ответы можно поискать [на вики](https://wiki.yandex-team.ru/cloud/compute/duty/how-to-support/).\n",
            "Если найти ответ на вики не получилось или у вас другой вопрос, то заведите тикет из [формы](https://st.yandex-team.ru/createTicket?queue=COMPUTESUPPORT&_form=76344),",
            " и дежурный поможет вам.\n",
            "**Если у вас факап,** то [вот здесь](https://wiki.yandex-team.ru/cloud/compute/duty/how-to-support/v-sluchae-fakapa/) описано, кого и в какой ситуации призывать.",
        ],
    }

    use_webhook = env.bool("USE_WEBHOOK", False)
    if use_webhook:
        telegram_webhook_url = env.str("TELEGRAM_WEBHOOK_URL", "")
        yandex_messenger_webhook_url = env.str("YANDEX_MESSENGER_WEBHOOK_URL", "")

    # It's some kind of hack. In production environment we have 3 instances with bot, one in each AZ (availability zone).
    # We have to run regular jobs (like a sending reminders about future duties) only from one of them.
    # So we use special environment variable $RUN_REGULAR_JOBS_ON_AZ with value "ru-central1-a" and check
    # that it's a substring of current instances' AZ.
    # And yes, this scheme doesn't work if there is more than one instance in one AZ.
    # In the future, we just need to make this place better.
    run_regular_jobs_on_az = env.str("RUN_REGULAR_JOBS_ON_AZ", "")
    if run_regular_jobs_on_az:
        run_regular_jobs = run_regular_jobs_on_az in get_current_az()
    else:
        # If environment variable is not set, then it's a local run.
        run_regular_jobs = True

    DUTY_SCORE_LIST = (
        "Все плохо 🙁  Давайте чинить процесс...",
        "Тяжело, но жить буду",
        "В целом норм...",
        "Легко!",
        "Незаметно 🙂",
    )

    # IM's meeting
    PING_CLOUDINC_FILTER = 'Queue: "YC Incidents" Tags: "inc-report:report-ready" tags: ! "inc-report:done" CLOUDINC."Meeting Date": <= today()'
    TODAY_CLOUDINC_FILTER = 'Queue: "YC Incidents" Tags: "inc-report:report-ready" tags: ! "inc-report:done" CLOUDINC."Meeting Date": today()'
    FUTURE_CLOUDINC_FILTER = 'Queue: "YC Incidents" Tags: "inc-report:report-ready" tags: ! "inc-report:done" CLOUDINC."Meeting Date": > today()'
    NO_ANSWER_CLOUDINC_FILTER = 'Queue: "YC Incidents" Tags: "inc-report:report-ready" tags: ! "inc-report:done" CLOUDINC."Meeting Date": < today()'

    shift_translate = {
        1: "завтра",
        2: "послезавтра",
        7: "понедельник",
    }

    WORKDAYS = tuple(range(5))
