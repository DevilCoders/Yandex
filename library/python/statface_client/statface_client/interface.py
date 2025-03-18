# coding: utf-8

from __future__ import division, absolute_import, print_function, unicode_literals

import logging
import six.moves.urllib.parse

from .base_client import BaseStatfaceClient
from .report import StatfaceReportAPI
from .report import StatfaceReportConfig
from .errors import StatfaceClientReportError
from .dictionary import StatfaceDictionary
from .constants import (
    STATFACE_BETA, STATFACE_PRODUCTION,
    STATFACE_BETA_FRONTEND, STATFACE_PRODUCTION_FRONTEND,
)


LOGGER = logging.getLogger('statface_client')


class StatfaceClient(BaseStatfaceClient):

    frontend_host = None

    def __init__(self, path_prefix="", **kwargs):
        if path_prefix != "" and path_prefix[-1] != "/":
            raise StatfaceClientReportError(
                "Incorrect path_prefix {}, must end with '/'".format(path_prefix)
            )
        self._path_prefix = path_prefix
        super(StatfaceClient, self).__init__(**kwargs)

    def __repr__(self):
        template = '<StatfaceClient {self.host} {self.username}>'
        return template.format(self=self)

    def get_report(self, path):
        return StatfaceReport(self, self._path_prefix + path)

    def get_new_report(self, path):
        report = self.get_report(path)
        if report.is_config_uploaded:
            raise StatfaceClientReportError(
                'report with path {} exists on server'.format(report.path)
            )
        return report

    def get_old_report(self, path):
        report = self.get_report(path)
        if not report.is_config_uploaded:
            raise StatfaceClientReportError(
                "report with path {} doesn't exist on server".format(
                    report.path
                )
            )
        return report

    def get_stat_dict(self, name):
        return StatfaceDictionary(self, name)


class BetaStatfaceClient(StatfaceClient):

    frontend_host = STATFACE_BETA_FRONTEND

    def __init__(self, **kwargs):
        super(BetaStatfaceClient, self).__init__(host=STATFACE_BETA, **kwargs)


class ProductionStatfaceClient(StatfaceClient):

    frontend_host = STATFACE_PRODUCTION_FRONTEND

    def __init__(self, **kwargs):
        super(ProductionStatfaceClient, self).__init__(host=STATFACE_PRODUCTION, **kwargs)


class StatfaceReport(object):  # pylint: disable=too-many-instance-attributes

    def __init__(self, statface_client, path):

        self._api = StatfaceReportAPI(statface_client, path)

        # methods
        self.download_config = self._api.download_config
        self.download_data = self._api.download_data
        self.upload_config = self._api.upload_config
        self.upload_data = self._api.upload_data
        self.simple_upload = self._api.simple_upload
        self.upload_yt_data = self._api.upload_yt_data
        self.fetch_distincts = self._api.fetch_distincts
        self.download_metadata = self._api.download_metadata
        self.merge_metadata = self._api.merge_metadata
        self.fetch_missing_dates = self._api.fetch_missing_dates
        self.fetch_available_dates_range = self._api.fetch_available_dates_range

        self._api.check_path_valid()

    def __repr__(self):
        template = '<StatfaceReport {self.path} @ {self._api.client.host}>'
        return template.format(self=self)

    @property
    def report_api_version(self):
        return self._api.report_api_version

    @report_api_version.setter
    def report_api_version(self, report_api_version):
        LOGGER.warning('Report API version overriden from %s to %s',
                       self.report_api_version,
                       report_api_version
                       )
        self._api.report_api_version = report_api_version

    @property
    def is_config_uploaded(self):
        return self._api.check_config_uploaded()

    @property
    def path(self):
        return self._api.path

    @property
    def title(self):
        """report title from server"""
        return self.download_config()['title']

    @property
    def config(self):
        """report data config from server"""
        raw_config = self.download_config()
        config = StatfaceReportConfig()
        config.update_from_dict(raw_config)
        return config

    def fetch_dates(self, scale):
        return self._api.fetch_distincts('fielddate', scale=scale)

    @property
    def scales(self):
        return self._api.fetch_scales()

    @property
    def report_url(self):
        host = self._api.client.host
        if not (host.startswith('https://') or host.startswith('http://')):
            host = self._api.client.DEFAULT_PROTOCOL + host
        host = host.replace('upload.', '')
        return six.moves.urllib.parse.urljoin(host, self.path)
