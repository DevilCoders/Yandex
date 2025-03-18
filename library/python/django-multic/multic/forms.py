# coding: utf-8
import six

from django import forms
from django.forms.forms import DeclarativeFieldsMetaclass
from django.core.validators import EMPTY_VALUES
if six.PY3:
    from django.utils.encoding import smart_text as smart_unicode
else:
    from django.utils.encoding import smart_unicode

from .resources import SearchResource
from . import DoesNotExistResources

if not SearchResource.register:
    raise DoesNotExistResources


class BarSeparatedValuesField(forms.CharField):
    """Поле, представляющее список строковых значений, разделенных вертикальной
    чертой.

    Преобразуется в список строк.
    Пример: //some_url?fields=id|code|name --> ['id', 'code', 'name']

    Параметр `unicode` определяет будут ли строки в результирующем списке
    юникодными или байтовыми.

    Параметр `permitted_values` принимает список допустимых значений для
    валидации.
    # TODO: легко переделывается в FooSeparatedValuesField
    """
    default_error_messages = {
        'invalid_values': u'Some of given values are not permitted: %s'
    }

    def __init__(self, unicode=True, permitted_values=None, *args, **kwargs):
        self.unicode = unicode
        self.permitted_values = permitted_values
        super(BarSeparatedValuesField, self).__init__(*args, **kwargs)

    def to_python(self, value):
        if value in EMPTY_VALUES:
            return []
        string_function = smart_unicode if self.unicode else str
        return list(map(string_function, value.split('|')))

    def validate(self, values_list):
        if self.permitted_values is None:
            return values_list

        diff = set(values_list) - set(self.permitted_values)
        if diff:
            raise forms.ValidationError(
                self.error_messages['invalid_values'] % ', '.join(diff))
        return values_list


class ResourceFieldsMetaclass(DeclarativeFieldsMetaclass):
    """Метакласс добавляет в форму поля вида `<recourcename>_fields`,
    представляющих собой список полей для любого типа подсказываемых сущностей.
    """
    def __new__(cls, name, bases, attrs):
        cls._search_resources = SearchResource.register
        for resource_name, resource in cls._search_resources.items():
            resource_fields = list(resource.get_available_fields())

            types_field = BarSeparatedValuesField(
                permitted_values=cls._search_resources.keys(),
            )
            resource_fields_field = BarSeparatedValuesField(
                required=False,
                unicode=True,
                permitted_values=resource_fields,
            )
            resource_id_field = forms.ChoiceField(
                required=False,
                choices=((v, v) for v in resource_fields),
            )

            field_dict = {
                'types': types_field,
                resource_name + '_fields': resource_fields_field,
                resource_name + '_id': resource_id_field,
            }

            attrs.update(field_dict)

        return super(ResourceFieldsMetaclass, cls).__new__(
            cls, name, bases, attrs)


class MulticompleteParamsValidator(six.with_metaclass(ResourceFieldsMetaclass, forms.Form)):
    """Форма для валидации и хранения параметров мультикомплита, метакласс
    динамически добавляет поля для перечисления списка полей доступных ресурсов.
    """

    q = forms.CharField(required=False)
    limit = forms.IntegerField(required=False, min_value=1)
