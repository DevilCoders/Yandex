# -*- coding: utf-8 -*-
import re
import pprint
import logging

import requests

from src.groupmap import TGroupToTagMapping
from src.transports.gencfg_api_transport import GencfgApiTransport


class YappyGencfgApiTransport(GencfgApiTransport):
    """
    Transport for generating yappy balancer config sections. Inherits from basic GencfgApiTransport and
    supports instances resolving by gencfg tags.
    """

    # Cannot use non-alpahanumeric symbols as delimiters due to the transformation of group names to lua variables
    GENCFG_TAG_DELIMITER = '__'
    MULTIGROUP_DELIMITER = '___'

    def __init__(self, *args, **kwargs):
        self.emit_loginfo = kwargs.pop('emit_loginfo', False)
        super(YappyGencfgApiTransport, self).__init__(*args, **kwargs)

    def test(self):
        TEST_GROUPS = ['{}_YAPPY_BALANCER'.format(loc) for loc in ('VLA', 'MAN', 'SAS')]
        TEST_TAGS = ['stable-104-r187', 'stable-104-r189']
        for group in TEST_GROUPS:
            gname = '{}{}{{}}'.format(group, self.GENCFG_TAG_DELIMITER)
            old_instances = self.get_group_instances(gname.format(TEST_TAGS[0]))
            new_instances = self.get_group_instances(gname.format(TEST_TAGS[1]))
            assert len(old_instances) != len(new_instances)

    def loginfo(self, message, message_data=None):
        if self.emit_loginfo:
            logging.info('<{}> {} => ({})'.format(
                self.__class__.__name__,
                message,
                pprint.pformat(message_data) if message_data is not None else 'NO DATA'
            ))

    def _extract_tag(self, group_name):
        try:
            group, tag = group_name.split(self.GENCFG_TAG_DELIMITER)
            tag = tag.replace('_', '-')
        except ValueError:
            group, tag = group_name, 'trunk'

        return group, self._convert_tag_name(tag)

    def _convert_tag_name(self, tagname):
        if re.match('tags/stable-(\d+)-r(\d+)', tagname):
            return tagname

        return super(YappyGencfgApiTransport, self)._convert_tag_name(tagname)

    def get_group_instances(self, group_name, gencfg_version=None):
        self.loginfo('Start resolving expression', group_name)
        groups = group_name.split(self.MULTIGROUP_DELIMITER)
        instances = []
        for group in groups:
            instances += self._get_group_instances(group, gencfg_version)
        return instances

    def _get_group_instances(self, group_name, gencfg_version=None):
        self.loginfo('Start resolving group', group_name)
        if (TGroupToTagMapping.ETypes.GROUP, group_name) not in self.instances_cache:
            group, tag = self._extract_tag(group_name)
            self.loginfo('Group missed in cache, client args are', dict(name=group_name, group=group, tag=tag))
            try:
                instances = self._gencfg_client.list_group_instances(group, tag)
            except requests.exceptions.HTTPError, e:
                raise Exception(
                    "Got HTTPError when getting instances for group {} in tag {}: {} (response content {})".format(
                        group_name, tag, str(e), e.response.content)
                )

            self.loginfo('Resolved data', dict(group=group, instances=instances))

            instances = self._instances_from_sepelib_instances(instances)

            self.instances_cache[(TGroupToTagMapping.ETypes.GROUP, group_name)] = instances

        return self.instances_cache[(TGroupToTagMapping.ETypes.GROUP, group_name)]

    def name(self):
        return "yappy"
