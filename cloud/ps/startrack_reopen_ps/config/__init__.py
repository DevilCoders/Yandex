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
        "default": {
            "condition": {},
            "action": {
                "waitForAnswer": {
                    "text": "Пользователь не отвечает нам уже неделю. Закрываю тикет.",
                    "summon": False,
                    "opening_status": "closed"
                },
                "inProgress": {
                    "text": """Коллеги, привет. Мы все еще ждем от вас ответа в этом тикете. 
        Если считаете что ваш ответ больше не требуется - удалите себя из призванных в тикет""",
                    "summon": False,
                    "opening_status": "open"
                },
                "awaitingDeployment": {
                    "text": "Связанные тикеты решены. Каков статус фикса?",
                    "summon": False,
                    "opening_status": "inProgress"
                },
                "watchingByDevelopers": {
                    "text": """Коллеги, привет. Мы все еще ждем от вас ответа в этом тикете.  
                    Если считаете что ваш ответ больше не требуется - удалите себя из призванных в тикет""",
                    "summon": False,
                    "opening_status": "inProgress"
                },
                "diagnostics": {
                    "text": """Коллеги, привет. Мы все еще ждем от вас ответа в этом тикете.  
                    Если считаете что ваш ответ больше не требуется - удалите себя из призванных в тикет""",
                    "summon": False,
                    "opening_status": "inProgress"
                },
                
            }
        }
    },
    "startrek": {
        "base_url": "https://st-api.yandex-team.ru/v2/issues",
        "filter": "filter=queue:CLOUDLINETWO&filter=status:inProgress,waitForAnswer,awaitingDeployment,watchingByDevelopers,diagnostics&perPage=1000",
        "rps_limit": {"get": 5, "post": 50},
    },
}
