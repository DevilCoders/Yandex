# -*- coding: utf-8 -*-

import json

from django import template
from django.core.validators import (MaxLengthValidator,
                                    MaxValueValidator, MinValueValidator)
from django.db import models
from django.db.models import Count, Q
from django.utils.safestring import mark_safe
from django.utils import timezone
from django.utils.translation import (string_concat, ugettext_noop,
                                      ugettext_lazy as _)
from south.modelsinspector import add_introspection_rules

from core.actions import qconstructor
from core.actions.snipextraparser import JsonFields
from core.hard.loghandlers import SteamLogger
from core.settings import ZERO_TIME, HIDE_BOLD_RE

MAX_TITLE_LENGTH = 100
MAX_UUID_LENGTH = 36
MAX_STORAGE_ID = 64
MAX_SNIPPET_ID = MAX_STORAGE_ID + 10
MAX_COMMENT = 500


add_introspection_rules([], ["^core\.models\.TinyJSONField"])


class TinyJSONField(models.Field):
    __metaclass__ = models.SubfieldBase

    def __init__(self, *args, **kwargs):
        super(TinyJSONField, self).__init__(*args, **kwargs)

    def db_type(self, connection):
        if connection.settings_dict['ENGINE'] == 'django.db.backends.mysql':
            return 'longtext'
        else:
            return 'text'

    def to_python(self, value):
        # None goes in migration
        if value is None:
            return {}
        # unicode goes from MySQL
        elif value.__class__ == unicode:
            try:
                return json.loads(value, encoding='utf-8')
            except (TypeError, ValueError) as exc:
                SteamLogger.warning('Json fails on load: %(error)s',
                                    type='JSON_LOAD_ERROR', error=exc)
                raise
        # dict goes from assignment in python
        elif value.__class__ == dict:
            return value
        # other types are wrong
        else:
            SteamLogger.warning(
                'Wrong data type for json field: %(data_type)s',
                type='JSON_LOAD_ERROR', data_type=value.__class__
            )
            raise TypeError('Wrong data type for Snippet.data field')

    def get_db_prep_value(self, value, connection, prepared):
        try:
            return json.dumps(value, encoding='utf-8', ensure_ascii=False)
        except (TypeError, ValueError) as exc:
            SteamLogger.warning("Json fails on dump: %(error)s",
                                type="JSON_SAVE_ERROR", error=exc)
            raise


class Country:
    RU = 'RU'
    UA = 'UA'
    TR = 'TR'
    AL = 'AL'
    HT = 'HT'
    CHOICES = (
        (RU, _('Russia')),
        (UA, _('Ukraine')),
        (TR, _('Turkey')),
        (AL, _('Albania')),
        (HT, _('Haiti')),
    )
    REGION_CHOICES = {
        RU: 213,
        UA: 143,
        TR: 11508,
        AL: 213,
        HT: 11508,
    }
    CHOICES_DICT = dict(CHOICES)


class User(models.Model):
    class Role:
        DEVELOPER = 'DV'
        ANALYST = 'AN'
        ASSESSOR = 'AS'
        AADMIN = 'AA'
        CHOICES = (
            (DEVELOPER, _('Developer')),
            (ANALYST, _('Analyst')),
            (ASSESSOR, _('Assessor')),
            (AADMIN, _('Assessor administrator')),
        )

    class Status:
        ACTIVE = 'A'
        WAIT_APPROVE = 'W'
        ANG = 'G'
        CHOICES = (
            (ACTIVE, _('Active')),
            (WAIT_APPROVE, _('Wait approve')),
            (ANG, _('ANG')),
        )

    login = models.CharField(max_length=32, default='')
    language = models.CharField(max_length=2,
                                choices=Country.CHOICES,
                                default=Country.RU)
    role = models.CharField(max_length=2,
                            choices=Role.CHOICES,
                            default=Role.ASSESSOR)
    status = models.CharField(max_length=2,
                              choices=Status.CHOICES,
                              default=Status.WAIT_APPROVE)
    yandex_uid = models.CharField(max_length=100, primary_key=True)

    def __unicode__(self):
        return self.login

    def role_str(self):
        return dict(User.Role.CHOICES)[self.role]

    def status_str(self):
        return dict(User.Status.CHOICES)[self.status]

    def language_str(self):
        return Country.CHOICES_DICT[self.language]


class QueryBin(models.Model):
    user = models.ForeignKey(User)
    title = models.CharField(max_length=MAX_TITLE_LENGTH)
    upload_time = models.DateTimeField()
    country = models.CharField(max_length=2, choices=Country.CHOICES)
    count = models.PositiveIntegerField()
    storage_id = models.CharField(max_length=MAX_STORAGE_ID)

    def __unicode__(self):
        return string_concat(self.title, ', ', self.country_str())

    def country_str(self):
        return Country.CHOICES_DICT[self.country]


class SnippetPool(models.Model):
    class Template:
        ANY = 'ANY'
        BASE = 'BAS'
        LIST = 'LST'
        RCA = 'RCA'
        CHOICES = (
            (ANY, _('Any')),
            (BASE, _('Base')),
            (LIST, _('List')),
            (RCA, _('RCA')),
        )
        CHOICES_DICT = dict(CHOICES)

    user = models.ForeignKey(User)
    title = models.CharField(max_length=MAX_TITLE_LENGTH)
    upload_time = models.DateTimeField()
    count = models.PositiveIntegerField()
    storage_id = models.CharField(max_length=MAX_STORAGE_ID, primary_key=True)
    tpl = models.CharField(max_length=3, choices=Template.CHOICES,
                           default=Template.ANY)

    def __unicode__(self):
        return string_concat(self.title, ' (', self.tpl_str(), ')')

    def tpl_str(self):
        return SnippetPool.Template.CHOICES_DICT[self.tpl]

    def can_delete(self):
        return self.can_delete_snippetpool(self.storage_id)

    @staticmethod
    def can_delete_snippetpool(snippetpool_id):
        return not TaskPool.objects.filter(
            Q(first_pool_id=snippetpool_id) |
            Q(second_pool_id=snippetpool_id)
        ).exists()


class Snippet(models.Model):
    class Template(SnippetPool.Template):
        # needed for template core/snippet_pair.html
        pass

    snippet_id = models.CharField(max_length=MAX_SNIPPET_ID, primary_key=True)
    data = TinyJSONField()

    def __unicode__(self):
        return self.snippet_id

    def can_delete(self):
        return self.can_delete_snippet(self.snippet_id)

    @staticmethod
    def can_delete_snippet(snippet_id):
        return not Task.objects.filter(
            Q(first_snippet=snippet_id) |
            Q(second_snippet=snippet_id)
        ).exists()


class TaskPool(models.Model):
    MAX_OVERLAP = 200

    class Type:
        BLIND = 'BLD'
        SIDE_BY_SIDE = 'SBS'
        PAGE_SEGMENTATION = 'SGM'
        CHOICES = (
            (BLIND, _('Blind')),
            (SIDE_BY_SIDE, _('SideBySide')),
            (PAGE_SEGMENTATION, _('Page segmentation')),
        )

    class TypePool:
        MULTI_CRITERIAL = 'MCR'
        RCA = 'RCA'
        CHOICES = (
            (MULTI_CRITERIAL, _('Multi criterial')),
            (RCA, _('RCA')),
        )

    class Status:
        ACTIVE = 'A'
        OVERLAPPED = 'O'
        FINISHED = 'F'
        DISABLED = 'D'
        CHOICES = (
            (ACTIVE, _('Active')),
            (OVERLAPPED, _('Overlapped')),
            (FINISHED, _('Finished')),
            (DISABLED, _('Disabled')),
        )

    title = models.CharField(
        max_length=MAX_TITLE_LENGTH,
        unique=True,
        error_messages={
            'unique': _('TaskPool with this title already exists!')
        }
    )
    create_time = models.DateTimeField()

    # TODO: delete for permit 'SGM' type
    if False:
        kind = models.CharField(max_length=3, choices=Type.CHOICES,
                                default=Type.BLIND)
    else:
        kind = models.CharField(max_length=3, choices=[
                                    ch for ch in Type.CHOICES
                                    if ch[0] != Type.PAGE_SEGMENTATION
                                ], default=Type.BLIND)
    kind_pool = models.CharField(max_length=3, choices=TypePool.CHOICES,
            default=TypePool.MULTI_CRITERIAL)

    status = models.CharField(max_length=1, choices=Status.CHOICES)
    priority = models.PositiveSmallIntegerField(default=1)
    overlap = models.PositiveSmallIntegerField(
        validators=[MaxValueValidator(MAX_OVERLAP)],
        default=1
    )
    ang_taskset_id = models.CharField(max_length=MAX_STORAGE_ID)
    first_pool = models.ForeignKey(SnippetPool, related_name='first_ref')
    first_pool_tpl = models.CharField(
        max_length=3, choices=SnippetPool.Template.CHOICES,
        default=SnippetPool.Template.ANY)
    second_pool = models.ForeignKey(SnippetPool, related_name='second_ref')
    second_pool_tpl = models.CharField(
        max_length=3, choices=SnippetPool.Template.CHOICES,
        default=SnippetPool.Template.ANY)
    count = models.PositiveIntegerField()
    pack_size = models.PositiveIntegerField(
        validators=[MinValueValidator(1)],
        default=1
    )
    deadline = models.DateField(default=ZERO_TIME.date())
    user = models.ForeignKey(User, null=True)

    def __unicode__(self):
        return self.title

    def kind_str(self):
        return dict(TaskPool.Type.CHOICES)[self.kind]

    def kind_pool_str(self):
        return dict(TaskPool.TypePool.CHOICES)[self.kind_pool]

    @staticmethod
    def snip_pool_str(title, tpl):
        return string_concat(title, ' (',
                             SnippetPool.Template.CHOICES_DICT[tpl], ')')

    def first_pool_str(self):
        return TaskPool.snip_pool_str(self.first_pool.title,
                                      self.first_pool_tpl)

    def second_pool_str(self):
        return TaskPool.snip_pool_str(self.second_pool.title,
                                      self.second_pool_tpl)

    def completed_count(self):
        return Task.objects.filter(
            qconstructor.complete_est_q(prefix='estimation__'),
            taskpool=self.id
        ).aggregate(completed=Count('estimation'))['completed']

    def tasks_status(self):
        complete_est_Q = qconstructor.complete_est_q(prefix='estimation__')
        regular_ests_count = Task.objects.filter(
            complete_est_Q,
            taskpool=self.id,
            status=Task.Status.REGULAR,
        ).aggregate(completed=Count('estimation'))['completed']

        inspection_ests_count_max_overlap = sum(
            map(
                lambda completed: min(completed, self.overlap),
                Task.objects.filter(
                    complete_est_Q,
                    taskpool=self.id,
                ).exclude(
                    status=Task.Status.REGULAR
                ).annotate(completed=Count('estimation')).values_list(
                    'completed', flat=True
                )
            )
        )
        return regular_ests_count + inspection_ests_count_max_overlap

    def overlap_count(self):
        return self.overlap * self.count

    def can_delete(self):
        return self.can_delete_taskpool(self.id)

    @staticmethod
    def can_delete_taskpool(taskpool_id):
        return not Estimation.objects.filter(
            task__taskpool_id=taskpool_id
        ).exists()


class TPCountry(models.Model):
    taskpool = models.ForeignKey(TaskPool)
    country = models.CharField(max_length=2, choices=Country.CHOICES)

    def for_ang(self):
        if self.country == Country.AL:
            return Country.RU
        if self.country == Country.HT:
            return Country.TR
        return self.country

    def __unicode__(self):
        return Country.CHOICES_DICT[self.country]


class TaskPack(models.Model):
    class Status:
        ACTIVE = 'A'
        TIMEOUT = 'T'
        FINISHED = 'F'
        CHOICES = (
            (ACTIVE, _('Active')),
            (TIMEOUT, _('Timeout')),
            (FINISHED, _('Finished')),
        )

    taskpool = models.ForeignKey(TaskPool)
    user = models.ForeignKey(User)
    last_update = models.DateTimeField()
    status = models.CharField(max_length=1, choices=Status.CHOICES)
    emailed = models.BooleanField(default=False)

    def __unicode__(self):
        return '%i-%i' % (self.id, self.taskpool_id)

    # better select_related taskpool to render TaskPack with this method
    def render_for_usertasks(self, tab):
        tpl = template.loader.get_template('core/pack_for_usertasks.html')
        complete_count = self.estimation_set.filter(
            status=Estimation.Status.COMPLETE
        ).count()
        rejected_count = self.estimation_set.filter(
            status=Estimation.Status.REJECTED
        ).count()
        return tpl.render(template.Context({
            'tp': self,
            'ests': self.estimation_set.order_by('status'),
            'tab': tab,
            'complete_count': complete_count,
            'rejected_count': rejected_count
        }))

    def render_for_current(self):
        return self.render_for_usertasks('current')

    def render_for_finished(self):
        return self.render_for_usertasks('finished')


class Tag(models.Model):
    label = models.CharField(max_length=20, primary_key=True)


class Task(models.Model):
    class Status:
        REGULAR = 'R'
        INSPECTION = 'I'
        COMPLETE = 'C'
        CHOICES = (
            (REGULAR, _('Regular')),
            (INSPECTION, _('Inspection')),
            (COMPLETE, _('Complete'))
        )

    taskpool = models.ForeignKey(TaskPool)
    first_snippet = models.ForeignKey(Snippet, related_name='first_ref')
    second_snippet = models.ForeignKey(Snippet, related_name='second_ref')
    request = models.TextField()
    region = models.PositiveIntegerField()
    status = models.CharField(max_length=1, choices=Status.CHOICES)
    shuffle = models.BooleanField(default=False)

    def __unicode__(self):
        return '%i-%s' % (self.id, self.request)

    def shuffle_coef(self):
        return -1 if self.shuffle else 1

    def snippets(self, ignore_shuffle=False):
        shuffle_coef = 1 if ignore_shuffle else self.shuffle_coef()
        return (self.first_snippet, self.second_snippet)[::shuffle_coef]

    def cur_tpls(self, ignore_shuffle=False):
        shuffle_coef = 1 if ignore_shuffle else self.shuffle_coef()
        return (self.taskpool.first_pool_tpl,
                self.taskpool.second_pool_tpl)[::shuffle_coef]

    def yandex_url(self):
        countries = [tpc.for_ang() for tpc in
                     self.taskpool.tpcountry_set.all()]
        domain = 'com.tr' if Country.TR in countries else 'ru'
        return (
            'http://www.yandex.%(domain)s/yandsearch?'
            'lr=%(region)d&text=%(request)s' % {'domain': domain,
                                                'region': self.region,
                                                'request': self.request}
        ).encode('utf-8')


class Estimation(models.Model):
    class Status:
        ASSIGNED = 'A'
        REJECTED = 'R'
        TIMEOUT = 'T'
        COMPLETE = 'C'
        SKIPPED = 'S'
        CHOICES = (
            (ASSIGNED, _('Assigned')),
            (REJECTED, _('Rejected')),
            (TIMEOUT, _('Timeout')),
            (COMPLETE, _('Complete')),
            (SKIPPED, _('Skipped')),
        )

    class Criterion:
        INFORMATIVITY = ugettext_noop('informativity')
        CONTENT_RICHNESS = ugettext_noop('content_richness')
        READABILITY = ugettext_noop('readability')
        MEDIA_CONTENT = ugettext_noop('media_content')
        NAMES_MCR = (
            INFORMATIVITY,
            CONTENT_RICHNESS,
            READABILITY,
        )
        NAMES_RCA = (
            READABILITY,
            CONTENT_RICHNESS,
            MEDIA_CONTENT,
        )
        OVERALL = ugettext_noop('overall')

    class SegmentationStage:
        CHOICE = ugettext_noop('choice')
        MERGE = ugettext_noop('merge')
        NAMES = (
            CHOICE,
            MERGE,
        )

    class SegmentName:
        AUX = 'AUX'
        AAD = 'AAD'
        ACP = 'ACP'
        AIN = 'AIN'
        ASL = 'ASL'

        DMD = 'DMD'
        DHC = 'DHC'
        DHA = 'DHA'
        DCT = 'DCT'
        DCM = 'DCM'

        LMN = 'LMN'
        LCN = 'LCN'
        LIN = 'LIN'

        CHOICES = (
            (AUX, ugettext_noop('Auxillary')),
            (AAD, ugettext_noop('Ads')),
            (ACP, ugettext_noop('Copyright')),
            (AIN, ugettext_noop('Input')),
            (ASL, ugettext_noop('Service links')),

            (DMD, ugettext_noop('Metadata')),
            (DHC, ugettext_noop('Useful header')),
            (DHA, ugettext_noop('Useless header')),
            (DCT, ugettext_noop('Text content')),
            (DCM, ugettext_noop('Media content')),

            (LMN, ugettext_noop('Menu')),
            (LCN, ugettext_noop('Content links')),
            (LIN, ugettext_noop('Internal navigation')),
        )

        HOTKEYS = (
            (AUX, 49),
            (AAD, 50),
            (ACP, 51),
            (AIN, 52),
            (ASL, 53),

            (DMD, 54),
            (DHC, 55),
            (DHA, 56),
            (DCT, 57),
            (DCM, 48),

            (LMN, 81),
            (LCN, 87),
            (LIN, 69),
        )

        COLORS = {
            'A': '#ff0000',
            'D': '#00ff00',
            'L': '#0000ff',
        }

        @staticmethod
        def grouped_names():
            res = []
            for seg_name, label in Estimation.SegmentName.CHOICES:
                if not res or seg_name[0] == res[0][0][0]:
                    res.append((seg_name, label,
                                Estimation.SegmentName.COLORS[seg_name[0]]))
                else:
                    yield res
                    res = [(seg_name, label,
                            Estimation.SegmentName.COLORS[seg_name[0]])]
            yield res

    VALUE_NAMES = (
        (-1, ugettext_noop('left')),
        (0, ugettext_noop('both')),
        (1, ugettext_noop('right')),
    )
    VALUE_NAMES_DICT = dict(VALUE_NAMES)

    VALUE_ICONS = (
        (-1, 'icon-arrow-left icon-red'),
        (0, 'icon-resize-horizontal'),
        (1, 'icon-arrow-right icon-blue'),
    )

    DEFAULT_VALUE = {}

    HOTKEYS = {
        Criterion.INFORMATIVITY: (81, 87, 69),      # q, w, e
        Criterion.CONTENT_RICHNESS: (81, 87, 69),   # q, w, e
        Criterion.READABILITY: (81, 87, 69),        # q, w, e
        Criterion.MEDIA_CONTENT: (81, 87, 69),      # q, w, e
    }

    TIMES = {
        Criterion.INFORMATIVITY: 'i_time',
        Criterion.CONTENT_RICHNESS: 'c_time',
        Criterion.READABILITY: 'r_time',
        Criterion.MEDIA_CONTENT: 'i_time',
    }

    task = models.ForeignKey(Task)
    user = models.ForeignKey(User)
    create_time = models.DateTimeField()
    start_time = models.DateTimeField()
    complete_time = models.DateTimeField()
    # time of first task rendering, needed to compensate start_time changes
    rendering_time = models.DateTimeField(default=ZERO_TIME)
    # time of first informativity posting
    i_time = models.DateTimeField(default=ZERO_TIME)
    # time of first content-richness posting
    c_time = models.DateTimeField(default=ZERO_TIME)
    # time of first readability posting
    r_time = models.DateTimeField(default=ZERO_TIME)
    status = models.CharField(max_length=1, choices=Status.CHOICES)
    json_value = TinyJSONField(null=True)
    comment = models.TextField(validators=[MaxLengthValidator(MAX_COMMENT)])
    answer = models.TextField(validators=[MaxLengthValidator(MAX_COMMENT)])
    taskpack = models.ForeignKey(TaskPack, null=True, blank=True,
                                 on_delete=models.SET_NULL)
    correction = models.ForeignKey('core.Correction', null=True, blank=True,
                                   on_delete=models.SET_NULL)

    tags = models.ManyToManyField(Tag, through='Estimation_Tag')

    def __unicode__(self):
        return string_concat(_('Estimation'), ' ', str(self.id))

    def shuffle_coef(self):
        return self.task.shuffle_coef()

    # gets dict {criterion_name: str(criterion_value)}
    # converts str to int, shuffles if not told to ignore
    def pack_value(self, est_dict, ignore_shuffle=False):
        shuffle_coef = 1 if ignore_shuffle else self.shuffle_coef()
        return {crit: shuffle_coef * int(est_dict[crit])
                for crit in est_dict
                if crit in self.get_criterions_names()}

    # returns list of tuples (criterion_name, human-readable criterion_value)
    def unpack_value(self, need_shuffle=True):
        return zip(
            self.get_criterions_names(),
            [self.VALUE_NAMES_DICT[d]
             for d in self.digits(shuffle=need_shuffle)]
        )

    def get_criterions_names(self):
        if self.task.taskpool.kind_pool == TaskPool.TypePool.RCA:
            return self.Criterion.NAMES_RCA
        return self.Criterion.NAMES_MCR

    # returns dict {criterion_name: human-readable criterion_value
    #                               with actual corrections}
    def corrected_values(self):
        corr_digits = self.corrected_digits()
        return {
            self.get_criterions_names()[i]:
            self.VALUE_NAMES_DICT[corr_digits[i]]
            for i in range(len(self.get_criterions_names()))
        }

    def digits(self, shuffle=False):
        shuffle_coef = self.shuffle_coef() if shuffle else 1
        return [self.json_value[crit] * shuffle_coef
                for crit in self.get_criterions_names()]

    def corrected_digits(self):
        self_digits = self.digits()
        if self.task.status != Task.Status.COMPLETE:
            return self_digits
        aadmin_digits = self.correction.aadmin_est.digits()
        return [aadmin_digits[i] if ((1 << i) & self.correction.errors)
                else self_digits[i]
                for i in range(len(self.get_criterions_names()))]

    def correction_by_aadmin_est(self, reference_est):
        for corr in reference_est.aadmin_corrections.all():
            if (
                corr.assessor_est_id == self.pk and
                corr.time > reference_est.complete_time
            ):
                return corr
        return Correction(errors=Correction.ALL_ERRORS,
                          status=Correction.Status.SAVED)

    def get_elapsed_time(self):
        if self.status not in (Estimation.Status.ASSIGNED,
                               Estimation.Status.SKIPPED):
            return int((self.complete_time - self.start_time).total_seconds())
        cur_time = timezone.now()
        if self.start_time == ZERO_TIME:
            self.start_time = cur_time
            self.save()
        if self.rendering_time == ZERO_TIME:
            self.rendering_time = cur_time
            self.save()
        return int((cur_time - self.start_time).total_seconds())

    def get_cur_criterion(self):
        for crit in self.get_criterions_names()[::-1]:
            if crit not in self.json_value:
                return crit
        return None

    def get_snippets(self, shuffle=True, b_tags=True, url=True):
        snippets = self.task.snippets(ignore_shuffle=not shuffle)
        if not b_tags:
            for snippet in snippets:
                snippet.data['title'] = HIDE_BOLD_RE.sub(
                    '', snippet.data['title']
                )
                snippet.data['text'] = [
                    HIDE_BOLD_RE.sub('', fragment)
                    for fragment in snippet.data['text']
                ]

                snippet.data[JsonFields.HEADLINE] = HIDE_BOLD_RE.sub(
                    '', snippet.data.get(JsonFields.HEADLINE, '')
                )
                snippet.data[JsonFields.HILITEDURL] = HIDE_BOLD_RE.sub(
                    '', snippet.data.get(JsonFields.HILITEDURL, '')
                )
                snippet.data[JsonFields.URLMENU] = [
                    [link, HIDE_BOLD_RE.sub('', title)]
                    for link, title in snippet.data.get(JsonFields.URLMENU, [])
                ]
        if not url:
            # don't show any url in template
            for snippet in snippets:
                snippet.data['url'] = ''
        return snippets

    def cur_tpls(self, shuffle=True):
        return self.task.cur_tpls(ignore_shuffle=not shuffle)

    def is_timeout(self):
        return (self.status == Estimation.Status.TIMEOUT or
                (self.taskpack_id and
                 self.taskpack.status == TaskPack.Status.TIMEOUT))

    # check if Estimation either doesn't belong to pack
    #                     or belongs to FINISHED pack
    def is_visible(self):
        return (not self.taskpack_id or
                self.taskpack.status == TaskPack.Status.FINISHED)

    def render_for_usertasks(self, tab):
        tpl = template.loader.get_template('core/est_for_usertasks.html')
        return mark_safe('\n'.join(('<tr>', tpl.render(template.Context({
            'est': self,
            'tab': tab,
            'user': self.user
        })), '</tr>')))

    def render_for_current(self):
        return self.render_for_usertasks('current')

    def render_for_finished(self):
        return self.render_for_usertasks('finished')

    # if we need correction_values for statistics, export, etc. (COMPLETE Task)
    #   we need to pass actual_only=True.
    # else (aadmin watches his corrections on estimation_check view)
    #   we need to pass actual_only=False, reference_est=this aadmin estimation
    def get_correction_value(self, actual_only=True, reference_est=None):
        if actual_only:
            assert(self.task.status == Task.Status.COMPLETE)
            corr = self.correction
            reference_est = corr.aadmin_est
        else:
            assert(reference_est is not None)
            corr = self.correction_by_aadmin_est(reference_est)
        aadmin_digits = reference_est.digits(shuffle=True)
        digits = self.digits(shuffle=True)
        return [(self.get_criterions_names()[i],
                 Estimation.VALUE_NAMES_DICT[digits[i]],
                 Estimation.VALUE_NAMES_DICT[aadmin_digits[i]],
                 (digits[i] != aadmin_digits[i] and
                  bool(corr.errors & 1 << i)))
                for i in range(len(Estimation.VALUE_NAMES))]


class Correction(models.Model):
    class Meta:
        ordering = ('-time',)

    class Status:
        REPORTED = 'R'
        ACTUAL = 'A'
        SAVED = 'S'
        CHOICES = (
            (REPORTED, _('Reported')),
            (ACTUAL, _('Actual')),
            (SAVED, _('Saved'))
        )

    ALL_ERRORS = 7

    aadmin_est = models.ForeignKey(Estimation,
                                   related_name='aadmin_corrections')
    assessor_est = models.ForeignKey(Estimation,
                                     related_name='assessor_corrections')
    errors = models.PositiveIntegerField()
    time = models.DateTimeField()
    comment = models.TextField(validators=[MaxLengthValidator(MAX_COMMENT)])
    status = models.CharField(max_length=1, choices=Status.CHOICES)


class Estimation_Tag(models.Model):
    estimation = models.ForeignKey(Estimation)
    tag = models.ForeignKey(Tag)

    class Meta:
        unique_together = ('estimation', 'tag')


class MetricReport(models.Model):
    METRIC_CHOICES = (
        ('AVG', _('Average')),
        ('INT', _('Intersection'))
    )
    user = models.ForeignKey(User)
    kind = models.CharField(max_length=3, choices=METRIC_CHOICES)
    storage_id = models.CharField(max_length=MAX_STORAGE_ID)
    pools = models.ManyToManyField(SnippetPool,
                                   through='MetricReport_SnippetPool')


class MetricReport_SnippetPool(models.Model):
    report = models.ForeignKey(MetricReport)
    snippetpool = models.ForeignKey(SnippetPool)

    class Meta:
        unique_together = ('report', 'snippetpool')


class BackgroundTask(models.Model):
    class Type:
        CALC = 'C'
        UPLOAD = 'U'
        SERP = 'S'
        CHOICES = (
            (CALC, _('Calculation')),
            (UPLOAD, _('Upload')),
            (SERP, _('SERPDownload')),
        )

    celery_id = models.CharField(max_length=MAX_UUID_LENGTH, primary_key=True)
    title = models.CharField(max_length=MAX_TITLE_LENGTH)
    task_type = models.CharField(max_length=1, choices=Type.CHOICES)
    status = models.CharField(max_length=MAX_TITLE_LENGTH)
    start_time = models.DateTimeField()
    user = models.ForeignKey(User)
    storage_id = models.CharField(max_length=MAX_STORAGE_ID)

    def not_finished(self):
        return self.status not in ('SUCCESS', 'ERROR', 'STOPPED')


class ANGReport(models.Model):
    class Status:
        SENT = 'S'
        ACKNOWLEDGED = 'A'
        CHOICES = (
            (SENT, _('Sent')),
            (ACKNOWLEDGED, _('Acknowledged')),
        )

    celery_id = models.CharField(max_length=MAX_UUID_LENGTH)
    time = models.DateTimeField()
    taskpool = models.ForeignKey(TaskPool)
    revision_number = models.PositiveIntegerField()
    status = models.CharField(max_length=1, choices=Status.CHOICES)
    storage_id = models.CharField(max_length=MAX_STORAGE_ID)

    def __unicode__(self):
        return string_concat(
            dict(ANGReport.Status.CHOICES)[self.status], ', ',
            timezone.localtime(self.time).strftime('%H:%M:%S %d.%m.%Y')
        )


class Notification(models.Model):
    class Kind:
        QUESTION = 'Q'
        RECHECK = 'R'
        DEADLINE = 'D'
        CHOICES = (
            (QUESTION, _('Question')),
            (RECHECK, _('Recheck')),
            (DEADLINE, _('Deadline')),
        )

    MESSAGE_TEMPLATES = {
        Kind.QUESTION: 'core/notifications/question.html',
        Kind.RECHECK: 'core/notifications/recheck.html',
        Kind.DEADLINE: 'core/notifications/deadline.html',
    }

    kind = models.CharField(max_length=1, choices=Kind.CHOICES)
    user = models.ForeignKey(User)
    # related taskpack for DEADLINE notification
    taskpack = models.ForeignKey(TaskPack, null=True)
    # related estimation for RECHECK and QUESTION notifications
    estimation = models.ForeignKey(Estimation, null=True)

    def message(self):
        tpl = template.loader.get_template(self.MESSAGE_TEMPLATES[self.kind])
        return tpl.render(template.Context({'notif': self}))


class Syncronizer(models.Model):
    aadmin_email = models.DateField()
