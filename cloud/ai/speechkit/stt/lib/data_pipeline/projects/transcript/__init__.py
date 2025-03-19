import json

import toloka.client as toloka


def get_project(lang: str) -> toloka.Project:
    # Write instruction in Toloka edit project WYSIWYG interface and just paste it here.
    with open(f'instruction_{lang}.html') as f:
        instruction = f.read()

    project = toloka.Project(
        public_name={
            'ru-RU': 'Расшифровка коротких аудиозаписей',
            'kk-KK': 'Қысқаша аудиожазбаның мағынасын ашу',
        }[lang],
        public_description={
            'ru-RU': 'Запишите то, что слышите на записи по инструкции.',
            'kk-KK': 'Запишите речь, которую вы слышите на записи.',
        }[lang],
        private_comment={
            'ru-RU': '',
            'kk-KK': 'расшифровка на казахском',
        }[lang],
        public_instructions=instruction,
    )

    # Create UI and in/out spec in https://tb.yandex.net/editor and just paste it here.
    with open('interface.json') as f:
        config_str = f.read()
        config_str = config_str.replace('{{text_re}}', {
            'ru-RU': '^[а-яё ?]*$',
            'kk-KK': '^[а-яёәғқңөұүһі ?]*$',
        }[lang])
        config_str = config_str.replace('{{text_re_hint}}', {
            'ru-RU': 'Только русские буквы и знак вопроса.',
            'kk-KK': 'Только казахские буквы и знак вопроса.',
        }[lang])
        config = json.loads(config_str)
        interface = toloka.project.TemplateBuilderViewSpec(
            # TODO (TOLOKAKIT-467): пока вставка json конфига не работает через объект, валидации не будет
            # config=toloka.structure(json.load(f), toloka.project.template_builder.TemplateBuilder)
            config=config,
            settings={
                'showSubmit': True,
                'showFinish': True,
                'showTimer': True,
                'showReward': True,
                'showTitle': True,
                'showRoute': True,
                'showComplain': True,
                'showMessage': True,
                'showSubmitExit': True,
                'showFullscreen': True,
                'showInstructions': True,
            },
        )

    input_specification = {
        'id': toloka.project.StringSpec(required=True, hidden=True),
        'url': toloka.project.UrlSpec(required=True, hidden=True),
        'user_url': toloka.project.UrlSpec(required=True),
    }

    output_specification = {
        'text': toloka.project.StringSpec(required=False),
        'cls': toloka.project.StringSpec(required=True, allowed_values=['sp', 'mis', 'si']),
    }

    project.task_spec = toloka.project.task_spec.TaskSpec(
        input_spec=input_specification,
        output_spec=output_specification,
        view_spec=interface,
    )

    return project
