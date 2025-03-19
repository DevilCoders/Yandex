# import json

# with open("conf.json") as fd:
#     cfg = json.load(fd)


cfg = {
    "company_filters": {
        "priority": {"high": ["business", "premium"]},
        "owner_type": {
            "individual": [
                "personal",
                "<unknown>",
                "<no billing account>",
                "individual",
            ]
        },
    },
    "rules": {
        "tracker": {
            "condition": {
                "component": ["tracker", "wiki", "forms"]
            },
            "action": {
                "waitForAnswer": {
                    "text": "В этом тикете мы всё ещё ждём ответа от пользователя. Возможно, стоит узнать, как у него дела или закрыть тикет. Если сомневаешься, что делать, напиши в чат (2л) Трекер",
                    "summon": False,
                    "opening_status": "open1"
                },
                "needInfo": {
                    "text": """Коллеги, привет. Мы все еще ждем от вас ответа в этом тикете. Если есть вопросы, призовите @ilyabarkan.
        Если считаете что ваш ответ больше не требуется - удалите себя из призванных в тикет""",
                    "summon": False,
                    "opening_status": "open"
                },
                "awaitingDeployment": {
                    "text": "Тикет давно висит в статусе 'Ждет выкладки'. Проверь, не закрылись ли связанные таски. Возможно, пора ответить пользователю",
                    "summon": False,
                    "opening_status": "open"
                }
            }
        },
        "default": {
            "condition": {},
            "action": {
                "waitForAnswer": {
                    "text": "Пользователь нам не отвечает. Возможно, стоит узнать, как у него дела или закрыть тикет. Если не знаешь, что делать, напиши @ilyabarkan.",
                    "summon": False,
                    "opening_status": "open1"
                },
                "needInfo": {
                    "text": """Коллеги, привет. Мы все еще ждем от вас ответа в этом тикете. Если есть вопросы, призовите @ilyabarkan.
        Если считаете что ваш ответ больше не требуется - удалите себя из призванных в тикет""",
                    "summon": False,
                    "opening_status": "open"
                },
                "awaitingDeployment": {
                    "text": "Тикет долго не обновлялся. Проверь связанные тикеты, возможно, фикс уже выкачен и юзеру можно ответить. Если не знаешь, что делать, напиши @ilyabarkan.",
                    "summon": False,
                    "opening_status": "open"
                }
            }
        }
    },
    "startrek": {
        "base_url": "https://st-api.yandex-team.ru/v2/issues",
        "filter": "filter=queue:CLOUDSUPPORT&filter=status:needInfo,waitForAnswer,awaitingDeployment&perPage=100",
        "rps_limit": {"get": 5, "post": 50},
    },
}
