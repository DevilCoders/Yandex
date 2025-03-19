import json

import toloka.client as toloka


def get_project(lang: str) -> toloka.Project:
    # Write instruction in Toloka edit project WYSIWYG interface and just paste it here.
    with open(f'instruction_{lang}.html') as f:
        instruction = f.read()

    project = toloka.Project(
        public_name={
            'ru-RU': 'Проверка расшифровок коротких аудиозаписей',
            'kk-KK': 'Қысқаша аудиожазбаның мағынасын ашуды тексеру',
        }[lang],
        public_description={
            'ru-RU': 'Прослушайте аудио и проверьте, совпадает ли текст на аудио с предложенной расшифровкой.',
            'kk-KK': 'Прослушайте аудио и проверьте, совпадает ли текст на аудио с предложенной расшифровкой.',
        }[lang],
        private_comment={
            'ru-RU': '',
            'kk-KK': 'проверка расшифровок на казахском',
        }[lang],
        public_instructions=instruction,
    )

    # Create UI and in/out spec in https://tb.yandex.net/editor and just paste it here.
    with open('interface.json') as f:
        interface = toloka.project.TemplateBuilderViewSpec(
            # TODO (TOLOKAKIT-467): пока вставка json конфига не работает через объект, валидации не будет
            # config=toloka.structure(json.load(f), toloka.project.template_builder.TemplateBuilder)
            config=json.load(f),
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
        'text': toloka.project.StringSpec(required=False),
        'cls': toloka.project.StringSpec(required=True, allowed_values=['sp', 'mis', 'si']),
    }

    output_specification = {
        'ok': toloka.project.BooleanSpec(required=True),
        'cls': toloka.project.StringSpec(required=True, allowed_values=['sp', 'mis', 'si']),
        'multi': toloka.project.BooleanSpec(required=False),
        'comment': toloka.project.StringSpec(required=False),
    }

    project.task_spec = toloka.project.task_spec.TaskSpec(
        input_spec=input_specification,
        output_spec=output_specification,
        view_spec=interface,
    )

    return project
