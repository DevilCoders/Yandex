# -*- coding: utf-8 -*-

import datetime
import yaml

from library.python import resource
from jinja2 import Environment, PackageLoader, select_autoescape, FileSystemLoader

from cloud.iam.planning_tool.library.absences import AbsenceUtil
from cloud.iam.planning_tool.library.clients import (StartrekClient, StaffClient, WikiClient, GapClient, PasteClient,
                                                     WebsiteClient)
from cloud.iam.planning_tool.library.holidays import HolidayCalendar
from cloud.iam.planning_tool.library.worklog import WorklogUtil
from cloud.iam.planning_tool.library.utils import parse_duration


class ExprTag(yaml.YAMLObject):
    yaml_tag = '!expr'

    def __init__(self, value):
        self.value = value

    def __repr__(self):
        return f'ExprTag({self.value})'

    @classmethod
    def from_yaml(cls, loader, node):
        return ExprTag(node.value)

    @classmethod
    def to_yaml(cls, dumper, data):
        return dumper.represent_scalar(cls.yaml_tag, data.value)


yaml.SafeLoader.add_constructor('!expr', ExprTag.from_yaml)
yaml.SafeDumper.add_multi_representer(ExprTag, ExprTag.to_yaml)


class Config:
    def __init__(self, config_file, mds_on=True, token=None, mds_key_id=None, mds_key=None, template_path=None):
        self._yaml = self._load_config(config_file)
        if token is None:
            with open(self._yaml['auth']['token_file'], 'r') as f:
                token = f.read().strip()
        if mds_on and mds_key_id is None:
            mds_key_id = self._yaml['mds']['access_key_id']
        if mds_on and mds_key is None:
            with open(self._yaml['mds']['secret_key_file'], 'r') as f:
                mds_key = f.read().strip()

        self.holidays = HolidayCalendar(self._yaml['holidays'])
        self.startrek = StartrekClient(self._yaml['startrek'], token)
        self.staff = StaffClient(self._yaml['staff'], token)
        try:
            self.wiki = WikiClient(self._yaml['wiki'], token)
        except KeyError:
            pass
        self.gap = GapClient(self._yaml['gap'], token)
        self.paste = PasteClient(self._yaml['paste'], token)

        if mds_on:
            self.website = WebsiteClient(self._yaml['mds'], mds_key_id, mds_key)

        self.worklog_util = WorklogUtil(self._yaml['worklog'], self.holidays)
        self.absence_util = AbsenceUtil(self._yaml, self.gap, self.holidays)

        self._day_length = parse_duration(self._yaml['worklog'].get('day_length', '8h'))

        self.local_output = None  # по умолчанию не сохраняем в файл
        self.mds_saving = False
        rendering = self._yaml.get('rendering')
        if rendering is not None:
            if 'local' in rendering:
                local_output = rendering['local'].get('file_name', self.local_output)
                enabled = rendering['local'].get('enabled', True)
                if local_output is not None and enabled:
                    self.local_output = local_output
            if mds_on and 'mds' in rendering:
                self.mds_saving = rendering['mds'].get('enabled', self.mds_saving)
        if template_path is not None:
            loader = FileSystemLoader(template_path)
        else:
            loader = PackageLoader('cloud.iam.planning_tool.library')

        self.jinja: Environment = Environment(loader=loader,
                                              autoescape=select_autoescape(['html']),
                                              lstrip_blocks=True, trim_blocks=True)
        self.jinja.globals['set'] = set
        self.jinja.globals['dict'] = dict
        self.jinja.globals['print'] = print
        self.jinja.globals['max'] = max
        self.jinja.globals['min'] = min

        def format_work_time(delta):
            if not delta:
                return ''

            days, remainder = divmod(delta.total_seconds(), self._day_length.total_seconds())
            return str(datetime.timedelta(days=days, seconds=round(remainder)))

        def format_hours(delta):
            if not delta:
                return ''

            hours, remainder = divmod(delta.total_seconds(), 3600)
            minutes, seconds = divmod(remainder, 60)
            return "{:d}:{:02d}:{:02d}".format(int(hours), int(minutes), round(seconds))

        def to_workdays(delta):
            if not delta:
                return 0

            return delta.total_seconds() / (3600 * 8)

        def now():
            return datetime.datetime.now()

        self.jinja.globals['format_work_time'] = format_work_time
        self.jinja.globals['format_hours'] = format_hours
        self.jinja.globals['to_workdays'] = to_workdays
        self.jinja.globals['now'] = now

    def _load_config(self, config_file):
        base_config = yaml.safe_load(resource.find('common_config'))
        with open(config_file, encoding='utf8') as f:
            config = yaml.safe_load(f)
        self._move_config(config, base_config)
        return base_config

    # переносит поля из source в destination
    def _move_config(self, source, destination):
        for key in source.keys():
            if key in destination and isinstance(source[key], dict) and isinstance(destination[key], dict):
                self._move_config(source[key], destination[key])
            else:
                destination[key] = source[key]

    def _evaluate_node(self, context, node):
        if type(node) == list:
            for i, item in enumerate(node):
                node[i] = self._evaluate_node(context, item)
        elif type(node) == dict:
            for key, value in node.items():
                node[key] = self._evaluate_node(context, value)
        elif type(node) == ExprTag:
            return eval(node.value, {}, context)

        return node

    def evaluate(self, context):
        return self._evaluate_node(context, self._yaml)

    def dump(self):
        return yaml.dump(self._yaml, default_flow_style=False, default_style='')
