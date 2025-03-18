import re
from datetime import datetime, time

from django import forms
from django.core import validators
from django.core.exceptions import ValidationError
from django.utils.encoding import smart_str
from django.utils.safestring import mark_safe


class GenericIPAddressFieldRus(forms.GenericIPAddressField):
    def __init__(self, *args, **kwards):
        super(GenericIPAddressFieldRus, self).__init__(*args, **kwards)
        msg = "Введите верный ipv4 или ipv6 адрес."
        self.default_error_messages["invalid"] = msg
        self.error_messages["invalid"] = msg

    def to_python(self, value):
        if isinstance(value, str):
            value = smart_str(value).strip()
        return value


class ExpireWidget(forms.MultiWidget):
    def __init__(self, attrs=None):
        widgets = (
            forms.DateInput(attrs={"placeholder": "yyyy-mm-dd", "class": "form-control"}),
            forms.TimeInput(attrs={"placeholder": "hh:MM:ss", "class": "form-control"}),
        )
        super(ExpireWidget, self).__init__(widgets, attrs)

    def decompress(self, value):
        if value:
            return [value.date(), value.time().replace(microsecond=0)]
        return [None, None]


class ExpireField(forms.SplitDateTimeField):
    widget = ExpireWidget

    def compress(self, data_list):
        if data_list:
            # Raise a validation error if time or date is empty
            # (possible if SplitDateTimeField has required=False).
            if data_list[0] in validators.EMPTY_VALUES:
                raise ValidationError(self.error_messages["invalid_date"])
            if data_list[1] in validators.EMPTY_VALUES:
                data_list[1] = time()
            return datetime.combine(*data_list)
        return None

    def to_python(self, value):
        value = super(ExpireField, self).to_python(value)
        if value and value < datetime.datetime.now():
            raise ValidationError("Это время уже в прошлом")
        return value


class SimpleRadioRenderer(forms.RadioSelect):
    def render(self):
        return mark_safe("\n".join(["<div>" + ("%s\n" % w) + "</div>" for w in self]))


class ExpirePeriodField(forms.RegexField):

    def __init__(self, max_length=None, min_length=None, error_message=None, *args, **kwargs):
        regex = re.compile("^(?:(?P<days>\\d+)(?:d),?)? *(?:(?P<hours>\\d+)(?:h),?)? *(?:(?P<minutes>\\d+)(?:m),?)?$")
        super(ExpirePeriodField, self).__init__(regex, *args, **kwargs)

    def clean(self, value):
        value = super(ExpirePeriodField, self).clean(value)
        if not value:
            return
        time_groups = self.regex.match(value).groupdict()
        for key in time_groups:
            if time_groups[key]:
                time_groups[key] = int(time_groups[key])
            else:
                time_groups[key] = 0
        days_sec = time_groups["days"] * 24 * 60 * 60
        hours_sec = time_groups["hours"] * 60 * 60
        minutes_sec = time_groups["minutes"] * 60
        return days_sec + hours_sec + minutes_sec
