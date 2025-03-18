# coding: utf-8

from __future__ import unicode_literals
import re
import sys

import six

from ids.services.inflector.errors import InflectorParseError, \
    InflectorBadFormatStringError, InflectorBadArguments


class InflectorResource(object):
    CASES_MAPPING = {
        'кто': 'им',
        'что': 'им',
        'кого': 'род',
        'чего': 'род',
        'кому': 'дат',
        'чему': 'дат',
        'кого-что': 'вин',
        'кем': 'твор',
        'чем': 'твор',
        'о ком': 'пр',
        'о чем': 'пр',
        'где': 'пр',
        'именительный': 'им',
        'родительный': 'род',
        'дательный': 'дат',
        'винительный': 'вин',
        'творительный': 'твор',
        'предложный': 'пр'
    }

    CASE_ATTRIBUTES = {
        'nominative': 'им',
        'genitive': 'род',
        'accusative': 'вин',
        'dative': 'дат',
        'instrumental': 'твор',
        'prepositional': 'пр'
    }

    def __init__(self, answer):
        self.data = {}
        self.raw_data = answer
        self.parse_raw_answer()

    def parse_raw_answer(self):
        """
        Распарсить ответ склонятора
        @raises: InflectorParseError
        """
        if 'Form' in self.raw_data:
            for form_dict in self.raw_data['Form']:
                self.parse_case(form_dict['Grammar'], form_dict['Text'])
        else:
            raise InflectorParseError('Invalid inflector answer: ',
                                      self.raw_data)

    def get_case(self, case):
        """
        Вернуть падеж слова
        @param case: падеж
        @rtype: unicode
        @raises: InflectorBadArguments
        """
        try:
            if case in self.CASES_MAPPING:
                case = self.CASES_MAPPING[case]
            return self.data[case]
        except KeyError:
            raise InflectorBadArguments('Wrong case: ', case)

    def __getattr__(self, item):
        assert item in self.CASE_ATTRIBUTES, "Wrong case!"
        return self.data[self.CASE_ATTRIBUTES[item]]

    def parse_case(self, case_name, case):
        """
        Функция парсинга элемента падежа в ответе склонятора
        @param case_name: падеж
        @param case: соответствующая form_name часть ответа склонятора

        @rtype: None
        """
        self.data[case_name] = case

    def __getitem__(self, case):
        return self.get_case(case)


class WordResource(InflectorResource):
    def __repr__(self):
        return "WordResource: [{0}]".format(
            "|".join(
                ["{0}={1}".format(form, value)
                 for (form, value) in six.iteritems(self.data)]
            )
        ).encode(sys.stdout.encoding)


class TitleResource(InflectorResource):
    def parse_case(self, case_name, case):
        """
        Функция парсинга элемента падежа в ответе склонятора
        Сохраняет в self.data[form_name] dict с ключами
        [firstname, secondname, middlename]

        @param case_name: падеж
        @param case: соответствующая form_name часть ответа склонятора

        @rtype: None
        """
        pattern = r'persn{([^}]+)}famn{([^}]+)}patrn{([^}]+)?}gender{(?:m|f)}'
        match = re.match(pattern, case)
        if not match:
            raise InflectorParseError("Can't parse form: ", case)
        else:
            values = (value or '' for value in match.groups())
            dict_result = dict(zip(('first_name', 'last_name', 'middle_name'),
                                   values))
            self.data[case_name] = dict_result

    def format(self, fmt_string=None, case="им"):
        form_data = self.get_case(case)
        return self._format(form_data, fmt_string)

    def _format(self, data, fmt_string=None):
        if fmt_string is None:
            fmt_string = "{first_name} {last_name}"
        try:
            return fmt_string.format(**data)
        except KeyError:
            raise InflectorBadFormatStringError("Bad format string: ", format)

    def __repr__(self):
        return "TitleResource: [{0}]".format(
            "|".join(
                ["{0}={1}".format(form, self.format(form=form))
                 for form in self.data]
            )
        ).encode(sys.stdout.encoding)
