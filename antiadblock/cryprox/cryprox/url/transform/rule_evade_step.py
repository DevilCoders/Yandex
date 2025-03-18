# coding=utf-8
import re2
from library.python import resource

GENERIC_RULE_EVASION_SYMBOL = '$'

GENERIC_RULES = [rule_line for rule_line in resource.find('url/transform/rule_evade_list').splitlines() if not rule_line.startswith('#')]


# метод необходим для теста "test_evader_evades", чтоб подменить правила
def construct_forward_evade_regex(rules_list):
    return re2.compile('|'.join([re2.escape(rule) for rule in rules_list]), re2.IGNORECASE)  # rules are case insensitive!


def replacement(match):
    """
    Вставляет знак $ для избегания generic-правил на url-ы
    :param match: 'запрещенная' последовательность в url
    :return: чем заменить её
    """
    match_group = match.group()
    return match_group[:1] + GENERIC_RULE_EVASION_SYMBOL + match_group[1:]


generic_forward_evade_regex = construct_forward_evade_regex(GENERIC_RULES)


def encode(encrypted_url, forward_evade_regex=generic_forward_evade_regex):
    """
    https://st.yandex-team.ru/ANTIADB-1065
    В зашифрованной ссылке после base-64 могут встретиться нежелательные последовательности букв и чисел.
    Для избегания generic-правил на url-ы, эта конструкция изменяет нежелательные последовательности в урле, вставляя в них символ $.

    Подразумевается, что в правилах нет этого символа и что в зашифрованной части ссылки этого символа опять-таки нет.
    Это сильно упрощает логику: расшифровывающий код удаляет символ $ и все остается как было.
    """

    return forward_evade_regex.sub(replacement, encrypted_url)
