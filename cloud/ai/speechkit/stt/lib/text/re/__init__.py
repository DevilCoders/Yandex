import re
import typing

from dataclasses import dataclass


@dataclass
class RegexpSuite:
    lang: str
    match_re: re.Pattern
    clean_re: re.Pattern
    toloka_match_re: re.Pattern
    toloka_clean_re: re.Pattern


regexps = [
    RegexpSuite(
        lang=lang,
        match_re=re.compile(f'^[{valid_chars} ]*$'),
        toloka_match_re=re.compile(f'^[{valid_chars} ?]*$'),
        clean_re=re.compile(f'[^{valid_chars} ]'),
        toloka_clean_re=re.compile(f'[^{valid_chars} ?]'),
    )
    for lang, valid_chars in
    (
        ('ru-RU', 'а-яё'),
        ('kk-KK', 'а-яёіғқңүұһәө'),
        ('en-US', 'a-z\''),
        ('tr-TR', 'a-zçöüğış'),
        ('fr-FR', 'a-zàâæçéèêëîïôœùûüÿ\''),
        ('de-DE', 'a-zäöüß\''),
        ('fi-FI', 'a-zäåöšž'),
        ('sv-SV', 'a-zåäö'),
        ('az-AZ', 'abcçdeəfgğhxıijkqlmnoöprsştuüvyz'),
        ('he-HE', 'ץףךסטגןצזפקםדכחנעתבשמלראיוה')
    )
]

lang_to_regexp = {r.lang: r for r in regexps}
space_regexp = re.compile(r'\s+')


def match(text: str, lang: str, toloka: bool = False) -> typing.Tuple[bool, re.Pattern]:
    regexp_suite = get_regexp_suite(lang)
    regexp = regexp_suite.toloka_match_re if toloka else regexp_suite.match_re
    return re.match(regexp, text) is not None, regexp


def validate(text: str, lang: str, toloka: bool = False):
    matched, regexp = match(text, lang, toloka)
    if not matched:
        raise ValueError(f'text "{text}" does not match regexp {regexp} for language {lang}')


def clean(text: str, lang: str, toloka: bool = False) -> typing.Tuple[str, re.Pattern]:
    text = text.lower()
    regexp_suite = get_regexp_suite(lang)
    regexp = regexp_suite.toloka_clean_re if toloka else regexp_suite.clean_re
    cleaned_text = regexp.sub(' ', text)
    no_redundant_ws_text = space_regexp.sub(' ', cleaned_text).strip()
    return no_redundant_ws_text, regexp


def get_regexp_suite(lang: str) -> RegexpSuite:
    if lang not in lang_to_regexp:
        raise ValueError(f'no regexp suite for language {lang}')
    return lang_to_regexp[lang]
