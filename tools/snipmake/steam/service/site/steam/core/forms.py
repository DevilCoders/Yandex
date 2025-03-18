# -*- coding: utf-8 -*-

import os
import re
import urlparse

from django import forms, template
from django.db.models import Count
from django.core import validators
from django.core.exceptions import ValidationError
from django.utils import timezone
from django.utils.translation import ugettext_lazy as _

from ui.settings import DATE_INPUT_FORMATS

from core import settings
from core.models import (BackgroundTask, QueryBin, SnippetPool, TaskPool,
                         User, Estimation, Country)
from core.models import MAX_COMMENT, MAX_TITLE_LENGTH


### widgets ###


class FileUploadWidget(forms.FileInput):

    upload_type = ''
    fileinput_value = ''

    def __init__(self, attrs={}):
        try:
            self.upload_type = attrs['upload_type']
        except KeyError:
            pass
        return super(FileUploadWidget, self).__init__(attrs)

    def render(self, name, value, attrs):
        attrs['style'] = \
            'position: relative; left: -100px; z-index: 2; ' \
            'opacity: 0; width: 100px;'
        fileinput = super(FileUploadWidget, self).render(name,
                                                         self.fileinput_value,
                                                         attrs)
        tpl = template.loader.get_template('core/uploadwidget.html')
        return tpl.render(template.Context({
            'fileinput': fileinput,
            'name': name,
            'value': '' if value is None else value,
            'filename': self.fileinput_value,
            'upload_type': self.upload_type
        }))

    def value_from_datadict(self, data, files, name):
        self.fileinput_value = data.get('_'.join(('fileinput', name)))
        try:
            filename = data['_'.join(('tmp', name))]
            if (
                re.search(r'^(?:\w|_)+$', filename) and
                os.path.isfile(os.path.join(settings.TEMP_ROOT, filename))
            ):
                return filename
        except KeyError:
            pass
        return None


class CriterionWidget(forms.RadioSelect):
    def __init__(self, attrs={}):
        return super(CriterionWidget, self).__init__(attrs)

    def render(self, name, value, attrs):
        tpl = template.loader.get_template('core/criterionwidget.html')
        try:
            value = int(value)
        except (ValueError, TypeError):
            value = None
        return tpl.render(template.Context({
            'name': name,
            'value': value,
            'value_names_with_hotkeys': zip(Estimation.VALUE_NAMES,
                                            Estimation.HOTKEYS[name]),
        }))


class SelectWidget(forms.Select):
    def __init__(self, attrs=None, choices=()):
        if not attrs:
            attrs = {}
        attrs['class'] = 'selectpicker'
        attrs['data-container'] = 'body'
        return super(SelectWidget, self).__init__(attrs, choices)

    def render(self, name, value, attrs):
        return super(SelectWidget, self).render(name, value, attrs)


class MultipleSelectWidget(forms.SelectMultiple):
    def __init__(self, attrs=None, choices=()):
        return super(MultipleSelectWidget, self).__init__(attrs, choices)

    def render(self, name, value, attrs):
        selects = []
        if not value:
            attrs['id'] = '_'.join(('id', name, '0'))
            selects.append(SelectWidget(
                attrs=self.attrs, choices=self.choices,
            ).render(name, '', attrs))
        else:
            for (val_id, val) in enumerate(value):
                attrs['id'] = 'id_%s_%d' % (name, val_id)
                selects.append(SelectWidget(
                    attrs=self.attrs, choices=self.choices,
                ).render(name, val, attrs))
        tpl = template.loader.get_template('core/mselectwidget.html')
        return tpl.render(template.Context({
            'selects': selects,
            'name': name,
        }))


class DateWidget(forms.DateInput):
    MONTH_NAMES = (
        _('January'),
        _('February'),
        _('March'),
        _('April'),
        _('May'),
        _('June'),
        _('July'),
        _('August'),
        _('September'),
        _('October'),
        _('November'),
        _('December'),
    )

    DAY_NAMES = (
        _('Su'),
        _('Mo'),
        _('Tu'),
        _('We'),
        _('Th'),
        _('Fr'),
        _('Sa'),
    )

    def __init__(self, attrs={}):
        return super(DateWidget, self).__init__(attrs)

    def render(self, name, value, attrs):
        dateinput = super(DateWidget, self).render(
            name, value, attrs
        )
        tpl = template.loader.get_template('core/datewidget.html')
        return tpl.render(template.Context({
            'dateinput': dateinput,
            'month_names': self.MONTH_NAMES,
            'day_names': self.DAY_NAMES
        }))


class AnnotatedTextInput(forms.TextInput):
    def __init__(self, attrs={}, annotation=''):
        self.annotation = annotation
        return super(AnnotatedTextInput, self).__init__(attrs)

    def render(self, name, value, attrs):
        textinput = super(AnnotatedTextInput, self).render(name, value, attrs)
        tpl = template.loader.get_template('core/annotatedtextinput.html')
        return tpl.render(template.Context({
            'textinput': textinput,
            'annotation': self.annotation
        }))


### fields ###


# typed multiple choice field without
# predefined iterable of choices
class NoIterableTypedMultipleChoiceField(forms.TypedMultipleChoiceField):
    def __init__(self, *args, **kwargs):
        return super(NoIterableTypedMultipleChoiceField, self).__init__(
            *args, **kwargs
        )

    def validate(self, value):
        if self.required and not value:
            raise ValidationError(self.error_messages['required'])

    def to_python(self, value):
        """
        Validates that the values can be coerced to the
        right type.
        """
        value = super(forms.TypedMultipleChoiceField, self).to_python(value)
        self.validate(value)
        if value == self.empty_value or value in validators.EMPTY_VALUES:
            return self.empty_value
        new_value = []
        for choice in value:
            try:
                new_value.append(self.coerce(choice))
            except (ValueError, TypeError, ValidationError):
                raise ValidationError(
                    self.error_messages['invalid_choice'] % {'value': choice}
                )
        return new_value


### forms ###


class AddUserForm(forms.Form):
    language = forms.ChoiceField(widget=SelectWidget,
                                 choices=Country.CHOICES,
                                 label=_('Country'))


class TaskPoolForm(forms.ModelForm):
    SNIPPET_POOLS_CHOICES = SnippetPool.objects.exclude(tpl=TaskPool.TypePool.RCA).order_by('-upload_time')
    RCA_POOLS_CHOICES = SnippetPool.objects.filter(tpl=TaskPool.TypePool.RCA).order_by('-upload_time')

    first_pool = forms.ModelChoiceField(widget=SelectWidget,
                                        queryset=SNIPPET_POOLS_CHOICES,
                                        label=_('First snippet pool'),
                                        required=False)
    second_pool = forms.ModelChoiceField(widget=SelectWidget,
                                         queryset=SNIPPET_POOLS_CHOICES,
                                         label=_('Second snippet pool'),
                                         required=False)
    first_pool_rca = forms.ModelChoiceField(widget=SelectWidget,
                                            queryset=RCA_POOLS_CHOICES,
                                            label=_('First pool'),
                                            required=False)
    second_pool_rca = forms.ModelChoiceField(widget=SelectWidget,
                                             queryset=RCA_POOLS_CHOICES,
                                             label=_('Second pool'),
                                             required=False)
    country = forms.MultipleChoiceField(widget=MultipleSelectWidget,
                                        choices=Country.CHOICES,
                                        label=_('Country'))
    ignore_bold = forms.BooleanField(required=False, label=_('Ignore bold'))
    kind_pool = forms.CharField(widget=forms.HiddenInput)

    def __init__(self, *args, **kwargs):
        action = kwargs.pop('action', None)
        super(TaskPoolForm, self).__init__(*args, **kwargs)
        instance = getattr(self, 'instance', None)

        self.fields['title'].label = _('Title')
        self.fields['kind'].label = _('Kind')
        self.fields['priority'].label = _('Priority')
        self.fields['first_pool_tpl'].label = _('First pool filter')
        self.fields['second_pool_tpl'].label = _('Second pool filter')
        self.fields['overlap'].label = _('Overlap')
        self.fields['deadline'].label = _('Deadline')
        self.fields['pack_size'].label = _('Pack size')

        self.fields['pack_size'].widget = AnnotatedTextInput(
            annotation=template.loader.get_template(
                'core/assessor_count_table.html'
            ).render(template.Context(
                {
                    'assessor_count': User.objects.filter(
                        role=User.Role.ASSESSOR
                    ).values(
                        'language'
                    ).annotate(
                        count=Count('yandex_uid')
                    )
                }
            ))
        )

        if instance and instance.pk:
            for exclude_field in ('kind', 'first_pool', 'first_pool_tpl',
                                  'second_pool', 'second_pool_tpl',
                                  'ignore_bold', 'first_pool_rca',
                                  'second_pool_rca', 'kind_pool'):
                self.fields.pop(exclude_field, None)
            if action == 'start':
                # for start shows only 'deadline' and 'pack_size'
                for exclude_field in ('title', 'country', 'priority',
                                      'overlap'):
                    self.fields.pop(exclude_field, None)
                if instance.ang_taskset_id:
                    # for not 1st start shows only 'deadline'
                    self.fields.pop('pack_size', None)
            else:
                self.fields['country'].initial = [
                    tpc.country for tpc in instance.tpcountry_set.all()
                ]
        else:
            for exclude_field in ('deadline', 'pack_size'):
                self.fields.pop(exclude_field, None)

    def clean_deadline(self):
        deadline = self.cleaned_data['deadline']
        if deadline <= timezone.localtime(timezone.now()).date():
            raise ValidationError(
                _('Deadline can not be less or equal than current date')
            )
        return deadline

    def clean_country(self):
        countries = self.cleaned_data['country']
        if len(countries) > len(set(countries)):
            raise ValidationError(_('Countries must differ'))
        return countries

    def clean(self):
        cleaned_data = super(TaskPoolForm, self).clean()
        instance = getattr(self, 'instance', None)
        if instance and instance.pk:
            #Next validation not work for editing form
            return cleaned_data
        # TODO: allow one snippet pool with different filters
        if cleaned_data.get('kind_pool') == TaskPool.TypePool.MULTI_CRITERIAL:
            first_pool_name = 'first_pool'
            second_pool_name = 'second_pool'
        elif cleaned_data.get('kind_pool') == TaskPool.TypePool.RCA:
            first_pool_name = 'first_pool_rca'
            second_pool_name = 'second_pool_rca'
        else:
            raise ValidationError(_('Unknown pool kind'))

        msg = _('Required field.')
        if cleaned_data.get(first_pool_name) is None:
            self._errors[first_pool_name] = self.error_class((msg,))
        if cleaned_data.get(second_pool_name) is None:
            self._errors[second_pool_name] = self.error_class((msg,))
        if cleaned_data.get(first_pool_name) == cleaned_data.get(second_pool_name):
            msg = _('Pools should be different!')
            self._errors[second_pool_name] = self.error_class((msg,))
        return cleaned_data

    class Meta:
        model = TaskPool
        fields = ('title', 'kind', 'country', 'priority',
                  'first_pool_tpl', 'second_pool_tpl',
                  'ignore_bold', 'overlap', 'pack_size', 'deadline', 'kind_pool')
        widgets = {
            'kind': SelectWidget,
            'first_pool_tpl': SelectWidget,
            'second_pool_tpl': SelectWidget,
            'pack_size': AnnotatedTextInput,
            'deadline': DateWidget,
        }


class UploadQueryBinForm(forms.Form):
    title = forms.CharField(max_length=MAX_TITLE_LENGTH, label=_('Title'))
    country = forms.ChoiceField(widget=SelectWidget, choices=Country.CHOICES,
                                label=_('Country'))
    file = forms.CharField(widget=FileUploadWidget, label=_('File'))

    def clean(self):
        cleaned_data = super(UploadQueryBinForm, self).clean()
        title = cleaned_data.get('title')
        country_code = cleaned_data.get('country')
        try:
            country = Country.CHOICES_DICT[country_code]
        except KeyError:
            pass
        if QueryBin.objects.filter(title=title, country=country_code).exists():
            msg = _('Query bin %(title)s for %(country)s already exists. Please specify other values.')
            raise ValidationError(msg % {'title': title, 'country': country})
        return cleaned_data

    def clean_file(self):
        filename = os.path.join(settings.TEMP_ROOT, self.data['tmp_file'])
        qb_file = open(filename)
        valid = True
        for line_num, line in enumerate(qb_file):
            line = line.rstrip()
            if not line or line[0] == '\t':
                valid = False
                break
            try:
                region = line.split('\t')[1]
            except IndexError:
                continue
            if region and not region.isdigit():
                valid = False
                break
        qb_file.close()
        if valid:
            return self.cleaned_data['file']
        raise ValidationError(_('Incorrect query bin: error in line %d') %
                              (line_num + 1))


class UploadSnippetPoolForm(forms.Form):

    HOST_CHOICES = (
        ('Choose', _('Choose')),
        ('yandex.ru', 'yandex.ru'),
        ('yandex.com', 'yandex.com'),
        ('yandex.com.tr', 'yandex.com.tr'),
        ('yandex.ua', 'yandex.ua')
    )
    title = forms.CharField(max_length=MAX_TITLE_LENGTH, label=_('Title'))
    querybin = forms.ModelChoiceField(widget=SelectWidget,
                                      queryset=QueryBin.objects.all(),
                                      required=False,
                                      label=_('Query bin'))
    host = forms.ChoiceField(widget=SelectWidget,
                             choices=HOST_CHOICES, required=False,
                             label=_('Host'))
    url = forms.URLField(required=False, label=_('URL'))
    wizard_url = forms.URLField(required=False, label=_('Wizard URL'))
    cgi_params = forms.CharField(required=False, label=_('CGI parameters'))
    pool_file = forms.CharField(
        widget=FileUploadWidget(attrs={'upload_type': 'upload'}),
        required=False,
        label=_('Pool file')
    )
    upload_type = forms.CharField(widget=forms.HiddenInput)
    json_pool = forms.BooleanField(required=False, label=_('JSON'))
    default_tpl = forms.ChoiceField(widget=SelectWidget,
                                    choices=SnippetPool.Template.CHOICES,
                                    required=False, label=_('Default template'))

    def clean_title(self):
        msg = ''
        title = self.cleaned_data['title']
        # if two uploads are started exactly at the same time
        # one of them will wait and give error
        downloads_in_progress = list(
            BackgroundTask.objects.select_for_update().filter(
                title=title,
                status__in=('PLANNED', 'DOWNLOADING', 'STORING')
            )
        )
        if downloads_in_progress:
            msg = _('Snippet pool %(title)s is already being processed by STEAM. Please specify other value.')
        if SnippetPool.objects.filter(title=title).exists():
            msg = _('Snippet pool %(title)s already exists. Please specify other value.')
        if msg:
            raise ValidationError(msg % {'title': title})
        return title

    def clean_querybin(self):
        upload_type = self.data.get('upload_type')
        querybin = self.cleaned_data['querybin']
        if upload_type == 'serp':
            if not querybin:
                raise ValidationError(_('Query bin not specified!'))
        return querybin

    def clean_host(self):
        upload_type = self.data.get('upload_type')
        host = self.cleaned_data['host']
        if upload_type == 'serp' and host not in dict(self.HOST_CHOICES):
            raise ValidationError(_('Incorrect host!'))
        return host

    def clean_url(self):
        upload_type = self.data.get('upload_type')
        host = self.data.get('host')
        url = self.cleaned_data['url']
        if (
            upload_type == 'serp' and host == self.HOST_CHOICES[0][0] and
            not url
        ):
            raise ValidationError(_('Choose URL!'))
        return url

    def clean_wizard_url(self):
        upload_type = self.data.get('upload_type')
        host = self.data.get('host')
        wizard_url = self.cleaned_data['wizard_url']
        if (
            upload_type == 'serp' and host == self.HOST_CHOICES[0][0] and
            not wizard_url
        ):
            raise ValidationError(_('Choose wizard URL!'))
        return wizard_url

    def clean_cgi_params(self):
        upload_type = self.data.get('upload_type')
        cgi_params = self.cleaned_data['cgi_params']
        if (
            upload_type == 'serp' and cgi_params and
            (urlparse.parse_qs(cgi_params) == {} or cgi_params.startswith('&'))
        ):
            raise ValidationError(_('Invalid CGI params!'))
        return cgi_params

    def clean_pool_file(self):
        upload_type = self.data.get('upload_type')
        pool_file = self.cleaned_data['pool_file']
        if upload_type == 'upload' and not pool_file:
            raise ValidationError(_('Pool file not specified!'))
        return pool_file

    def clean(self):
        self.cleaned_data = super(UploadSnippetPoolForm, self).clean()
        upload_type = self.cleaned_data.get('upload_type')
        if upload_type not in ('serp', 'upload'):
            raise ValidationError(_('Wrong upload type "%s"') % upload_type)
        return self.cleaned_data


class UploadTwoSnippetPoolsForm(forms.Form):
    MAX_SUFFIX_LENGTH = min(10, MAX_TITLE_LENGTH)
    MAX_PREFIX_LENGTH = MAX_TITLE_LENGTH - MAX_SUFFIX_LENGTH

    title_prefix = forms.CharField(max_length=MAX_PREFIX_LENGTH,
                                   label=_('Title prefix'))
    base_title_suffix = forms.CharField(max_length=MAX_SUFFIX_LENGTH,
                                        initial=' base',
                                        label=_('Base title suffix'))
    exp_title_suffix = forms.CharField(max_length=MAX_SUFFIX_LENGTH,
                                       initial=' exp',
                                       label=_('Exp title suffix'))
    two_pools_file = forms.CharField(widget=FileUploadWidget,
                                     label=_('Two pools file'))
    base_default_tpl = forms.ChoiceField(widget=SelectWidget,
                                         choices=SnippetPool.Template.CHOICES,
                                         label=_('Base default template'))
    exp_default_tpl = forms.ChoiceField(widget=SelectWidget,
                                        choices=SnippetPool.Template.CHOICES,
                                        label=_('Exp default template'))

    def clean(self):
        self.cleaned_data = super(UploadTwoSnippetPoolsForm, self).clean()
        title_prefix = self.cleaned_data.get('title_prefix', '')
        base_title_suffix = self.cleaned_data.get('base_title_suffix', '')
        exp_title_suffix = self.cleaned_data.get('exp_title_suffix', '')
        if base_title_suffix and base_title_suffix == exp_title_suffix:
            msg = _('Suffixes should be different!')
            self._errors['exp_title_suffix'] = self.error_class((msg,))
        else:
            base_title = title_prefix + base_title_suffix
            exp_title = title_prefix + exp_title_suffix
            msg = ''
            # if two uploads are started exactly at the same time
            # one of them will wait and give error
            downloads_in_progress = list(
                BackgroundTask.objects.select_for_update().filter(
                    title__in=(base_title, exp_title),
                    status__in=('PLANNED', 'DOWNLOADING', 'STORING')
                )
            )
            if downloads_in_progress:
                msg = _('Snippet pools %(title_prefix)s* are already being processed by STEAM. Please specify other values.')
            if SnippetPool.objects.filter(title__in=(base_title, exp_title)).exists():
                msg = _('Snippet pools %(title_prefix)s* already exist. Please specify other values.')
            if msg:
                raise ValidationError(msg % {'title_prefix': title_prefix})
        return self.cleaned_data


class EstimationForm(forms.Form):
    informativity = forms.ChoiceField(widget=CriterionWidget,
                                      choices=Estimation.VALUE_NAMES,
                                      required=False,
                                      label=Estimation.Criterion.INFORMATIVITY)
    content_richness = forms.ChoiceField(widget=CriterionWidget,
                                         choices=Estimation.VALUE_NAMES,
                                         required=False,
                                         label=Estimation.Criterion.CONTENT_RICHNESS)
    readability = forms.ChoiceField(widget=CriterionWidget,
                                    choices=Estimation.VALUE_NAMES,
                                    required=False,
                                    label=Estimation.Criterion.READABILITY)
    media_content = forms.ChoiceField(widget=CriterionWidget,
                                      choices=Estimation.VALUE_NAMES,
                                      required=False,
                                      label=Estimation.Criterion.MEDIA_CONTENT)
    comment = forms.CharField(widget=forms.Textarea(attrs={'placeholder': _('Comment'),
                                                           'rows': '1'}),
                              required=False,
                              max_length=MAX_COMMENT, label=_('Comment'))
    time_elapsed = forms.IntegerField(widget=forms.HiddenInput, initial=0)
    skipped = forms.CharField(widget=forms.HiddenInput, initial='False')
    linked = forms.CharField(widget=forms.HiddenInput, initial='0')

    def __init__(self, *args, **kwargs):
        self.current_criterion = kwargs.pop('criterion', None)
        self.kind_pool = kwargs.pop('kind_pool', TaskPool.TypePool.MULTI_CRITERIAL)
        if self.kind_pool == TaskPool.TypePool.RCA:
            self.criterion_names = Estimation.Criterion.NAMES_RCA
        else:
            self.criterion_names = Estimation.Criterion.NAMES_MCR
        super(EstimationForm, self).__init__(*args, **kwargs)

    def criterion_fields(self):
        if self.current_criterion and self.kind_pool == TaskPool.TypePool.MULTI_CRITERIAL:
            return (self[self.current_criterion],)
        return [self[name] for name in self.criterion_names]

    def clean_crit_val(self, crit_name):
        if self.current_criterion and self.current_criterion != crit_name and self.kind_pool == TaskPool.TypePool.MULTI_CRITERIAL:
            return
        skipped = self.data.get('skipped')
        crit_val = self.cleaned_data[crit_name]
        try:
            if (
                skipped != 'False' or
                int(crit_val) in Estimation.VALUE_NAMES_DICT
            ):
                return crit_val
        except ValueError:
            pass
        msg = _('Required field.')
        self._errors[crit_name] = self.error_class((msg,))

    def clean_informativity(self):
        if self.kind_pool == TaskPool.TypePool.RCA:
            return None
        return self.clean_crit_val(Estimation.Criterion.INFORMATIVITY)

    def clean_media_content(self):
        if self.kind_pool == TaskPool.TypePool.MULTI_CRITERIAL:
            return None
        return self.clean_crit_val(Estimation.Criterion.MEDIA_CONTENT)

    def clean_content_richness(self):
        return self.clean_crit_val(Estimation.Criterion.CONTENT_RICHNESS)

    def clean_readability(self):
        return self.clean_crit_val(Estimation.Criterion.READABILITY)

    def clean_comment(self):
        skipped = self.data.get('skipped')
        if skipped == 'Questioned' and not self.cleaned_data['comment']:
            raise ValidationError(_('This field is required!'))

    def clean(self):
        self.cleaned_data = super(EstimationForm, self).clean()
        if self.cleaned_data.get('skipped') not in ('Rejected', 'Questioned',
                                                    'False'):
            raise ValidationError(_('Could not determine if task is skipped'))
        return self.cleaned_data


class SegmentationForm(forms.Form):
    class BlockGuid:
        def __init__(self, data_str):
            self.split_id = ''
            self.guid = data_str
            if '_' in self.guid:
                (self.guid, self.split_id) = self.guid.split('_')
                if not self.split_id.isdigit():
                    raise ValueError('Incorrect split id: %s' % self.split_id)
            if not self.guid.isdigit():
                raise ValueError('Incorrect guid: %s' % self.guid)

        def __unicode__(self):
            if self.split_id:
                res = '_'.join((self.guid, self.split_id))
            else:
                res = self.guid
            return res

        def __str__(self):
            return self.__unicode__()

    class SegmentQualifier:
        def __init__(self, data_str):
            tokens = data_str.split(',', 2)
            if len(tokens) < 3:
                raise ValidationError(
                    'Two indexes and segment name are expected!'
                )
            self.indexes = map(int, tokens[:2])
            for index in self.indexes:
                if index < 0:
                    raise ValueError('Incorrect index: %d' % index)
            self.name = tokens[2]
            if self.name not in dict(Estimation.SegmentName.CHOICES):
                raise ValueError('Incorrect name: %s' % self.name)

        def __unicode__(self):
            return ','.join(map(str, self.indexes) + [self.name])

        def __str__(self):
            return self.__unicode__()

    choice = NoIterableTypedMultipleChoiceField(
        widget=forms.MultipleHiddenInput,
        empty_value=[],
        coerce=BlockGuid,
        required=True
    )
    merge = NoIterableTypedMultipleChoiceField(
        widget=forms.MultipleHiddenInput,
        empty_value=[],
        coerce=SegmentQualifier,
        required=True
    )
    comment = forms.CharField(
        widget=forms.Textarea(attrs={'placeholder': _('Comment'),
                                     'rows': '1'}),
        required=False,
        max_length=MAX_COMMENT, label=_('Comment')
    )
    time_elapsed = forms.IntegerField(widget=forms.HiddenInput, initial=0)
    skipped = forms.CharField(widget=forms.HiddenInput, initial='False')

    def __init__(self, *args, **kwargs):
        self.estimation = kwargs.pop('estimation', None)
        super(SegmentationForm, self).__init__(*args, **kwargs)

    def clean_comment(self):
        skipped = self.data.get('skipped')
        if skipped == 'Questioned' and not self.cleaned_data['comment']:
            raise ValidationError(_('This field is required!'))

    def clean(self):
        self.cleaned_data = super(SegmentationForm, self).clean()
        if self.cleaned_data.get('skipped') not in ('Rejected', 'Questioned',
                                                    'False'):
            raise ValidationError(_('Could not determine if task is skipped'))
        return self.cleaned_data


class AnswerEstimationForm(forms.Form):
    answer = forms.CharField(
        widget=forms.Textarea(attrs={'placeholder': _('Answer'),
                                     'rows': '1'}),
        max_length=MAX_COMMENT, label=_('Answer'),
        required=True
    )


class EmailForm(forms.Form):
    subject = forms.CharField(label=_('Subject'))
    message = forms.CharField(widget=forms.Textarea, label=_('Message'))

    def clean(self):
        self.cleaned_data = super(EmailForm, self).clean()
        if not self.get_recipients():
            raise ValidationError(_('No recipients specified'))
        return self.cleaned_data

    def get_recipients(self):
        recipients = []
        for field_name in self.data:
            recip = re.findall(r'^recip_(.*)$', field_name)
            if recip and self.data[field_name] == 'on':
                recipients.append(recip[0])
        if (
            User.objects.filter(login__in=recipients).count() <
            len(recipients)
        ):
            raise ValidationError(_('Not all recipients are in STEAM'))
        return recipients


class StatisticsForm(forms.Form):
    country = forms.ChoiceField(choices=(('All', _('All')),) +
                                Country.CHOICES,
                                widget=SelectWidget, required=True,
                                initial='All',
                                label=_('Country'))
    start_date = forms.DateField(widget=DateWidget, required=True,
                                 label=_('Start date'))
    end_date = forms.DateField(widget=DateWidget, required=True,
                               label=_('End date'))

    def clean_start_date(self):
        start_date = self.cleaned_data['start_date']
        if start_date > timezone.localtime(timezone.now()).date():
            raise ValidationError(
                _('Start date can not be greater than current date')
            )
        return start_date

    def clean_end_date(self):
        start_date = self.data.get('start_date')
        try:
            start_date = timezone.datetime.strptime(
                start_date, DATE_INPUT_FORMATS[0]
            ).date()
        except (ValueError, TypeError):
            pass
        end_date = self.cleaned_data['end_date']
        if end_date > timezone.localtime(timezone.now()).date():
            raise ValidationError(
                _('End date can not be greater than current date')
            )
        if start_date and end_date < start_date:
            raise ValidationError(
                _('End date can not be less than start date')
            )
        return end_date
