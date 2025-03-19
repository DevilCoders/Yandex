import re
from typing import Tuple, List


var_desc_regex = re.compile(r'\{.+?=.+?\}')
var_text_regex = re.compile(r'[^а-яёА-ЯЁ -]')
template_text_regex = re.compile(r'[^а-яёА-ЯЁ ,:.?!-]')
punct_regex = re.compile(r'[.?!]')


def validate_template(template: str, variable_sources: List[str]) -> Tuple[bool, str]:
    variable_sources = set(variable_sources)
    for var in var_desc_regex.findall(template):
        var_name, source_path = var[1:-1].split('=')
        if not source_path.endswith('.tsv'):
            return False, f'Описание переменной "{source_path}" имеет расширение, отличное от TSV'
        if source_path not in variable_sources:
            return False, f'Описание переменной "{source_path}" не было найдено'

    template = var_desc_regex.sub('', template).strip()
    max_len = 120

    if template and punct_regex.findall(template[:-1]):
        return False, f'В середине шаблона присутствуют знаки конца предложения'
    if len(template) > max_len:
        return False, f'Длина шаблона без учёта вставок переменных не должна превышать {max_len} символов'

    errors = set(template_text_regex.findall(template))
    if errors:
        return False, f'Шаблон содержит недопустимые символы: {errors}'

    return True, ''


def validate_variable(variable: str) -> Tuple[bool, str]:
    max_len = 40
    if len(variable) > max_len:
        return False, f'Длина переменной не должна превышать {max_len} символов'

    errors = set(var_text_regex.findall(variable))
    if errors:
        return False, f'Переменная содержит недопустимые символы: {errors}'
    return True, ''
