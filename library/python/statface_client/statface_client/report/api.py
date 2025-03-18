# coding: utf-8
# pylint: disable=too-many-lines
# TODO: refactor into feature-classes.

from __future__ import division, absolute_import, print_function, unicode_literals
import json
import logging
import uuid
import time
import types
import traceback
import six.moves.urllib.parse
import six.moves.urllib.error
import six

import requests.exceptions

from ..constants import (
    STATFACE_DATA_FORMATS, UPLOAD_FORMAT_PARAMS,
    SCALE_PARAMS, DOWNLOAD_SCALE_PARAMS,
    RETRIABLE_STATUSES, UNRETRIABLE_STATUSES,
    JSON_DATA_FORMAT, TSKV_DATA_FORMAT, PY_DATA_FORMAT,
    MAX_UPLOAD_BYTES, EMPTY_STR_SIZE,
    DO_NOTHING, JUST_ONE_CHECK, ENDLESS_WAIT_FOR_FINISH,
)
from ..errors import (
    StatfaceClientValueError, StatfaceReportConfigError,
    StatfaceInvalidReportPathError, StatfaceIncorrectResponseError,
    StatfaceClientDataDownloadError, StatfaceClientDataUploadError)
from .config import StatfaceReportConfig


logger = logging.getLogger('statface_client')


class StatfaceReportAPI(object):

    def __init__(self, statface_client, path, report_api_version='api'):
        # type: (StatfaceClient, basestring, basestring) -> None
        self.client = statface_client
        self.path = path
        self.report_api_version = report_api_version

    @property
    def _report_api_prefix(self):
        return {
            # Faked, not to be used.
            'v2': '_api/report/',
            # Default.
            'api': '_api/report/',
            # Effectively the same as default (the only difference is in logs).
            'v4': '_v4/report/',
        }[self.report_api_version]

    def _report_request(self, method, uri, host=None, **request_kwargs):
        uri = self._report_api_prefix + uri
        return self.client._request(method, uri, host, **request_kwargs)

    def _report_get(self, uri, host=None, **request_kwargs):
        uri = '{}/{}'.format(uri.rstrip('/'), self.path.lstrip('/'))
        return self._report_request('get', uri, host, **request_kwargs)

    def _report_post(self, uri, host=None, method='post', **request_kwargs):
        # Force parameters to go over `json` when no files are used, forbid specifying both.
        if 'json' in request_kwargs:
            assert 'data' not in request_kwargs
        else:
            data = request_kwargs.get('data')
            assert data is not None
            if ('files' not in request_kwargs and
                    not isinstance(data, (six.binary_type, six.text_type, types.GeneratorType))):
                request_kwargs['json'] = request_kwargs.pop('data')

        uri = '{}/{}'.format(uri.rstrip('/'), self.path.lstrip('/'))
        return self._report_request(method, uri, host, **request_kwargs)

    def _check_path_valid(self):
        response = self.client._request(
            'get',
            '/_v3/reportmenus/nav/check_name/',
            params=dict(uri=self.path),
        )
        return response.json()

    def check_path_valid(self):
        """raise error if report path is invalid"""
        if self.client._no_excess_calls:
            return

        # TODO: this method should not ever be useful.

        answer = self._check_path_valid()
        if not (answer.get('valid') or
                answer.get('exists')):
            raise StatfaceInvalidReportPathError(
                'Report path {} is invalid: {}'.format(
                    self.path,
                    answer.get('reason')))

    def _fetch_distincts(
            self,
            dimension,
            scale,
            request_kwargs=None,
            **statface_params):
        request_kwargs = request_kwargs or {}
        request_kwargs.setdefault('params', {})
        request_kwargs['params'].update(statface_params)
        request_kwargs['params']['dim'] = dimension
        request_kwargs['params']['scale'] = scale
        return self._report_get(
            'distincts',
            **request_kwargs
        )

    def fetch_distincts(
            self,
            dimension,
            scale,
            request_kwargs=None,
            **statface_params):
        return self._fetch_distincts(
            dimension,
            SCALE_PARAMS[scale],
            request_kwargs=request_kwargs,
            **statface_params
        ).json()

    def fetch_scales(self):
        return self._report_get(
            'scales'
        ).json()

    def _download_config(
            self,
            format='json'):  # pylint: disable=redefined-builtin
        """:param format: 'json' or 'yaml'"""
        return self._report_get('config', data=dict(format=format))

    def download_config(self):
        """Get deserialized report config from server."""
        response = self._download_config('json')

        try:
            result = response.json()
        except ValueError as exc:
            self._handle_non_correct_json_response(
                response, exc, exc_type=StatfaceClientDataDownloadError)
            raise

        return result

    def check_config_uploaded(self):
        response = self._report_get('config', raise_for_status=False)
        return response.status_code == 200

    def _process_upload_config_params(  # pylint: disable=too-many-branches,too-many-arguments
            self,
            config,
            config_format='config',
            title=None,
            request_kwargs=None,
            require_strings=None,
            do_logging=True):
        """
        :param config_format: 'top' or 'config' or 'json_config' or 'cube_config'

        Recommended value for `config_format` is 'config', which results in a JSON like this:

            {"config": {"title": "...", "user_config": {...}, "initial_owners": ..., ...}

        `user_config` in this case can be a JSON or YAML string.

        For `format_title='cube_config'` (deprecated), the `config` must be a user_config.

        `user_config` is a dict like this:

            {"dimensions": ..., "measures": ..., ...}

        """
        if isinstance(config, six.binary_type):
            config = config.decode('utf-8', errors='strict')

        request_kwargs = (request_kwargs or {}).copy()
        request_data = (request_kwargs.get('data') or {}).copy()
        if require_strings is None:
            require_strings = bool(request_kwargs.get('files'))

        config_formats = ('top', 'config', 'json_config', 'cube_config')
        if config_format not in config_formats:
            raise StatfaceClientValueError("Unknown `config_format` '{}'".format(config_format))
        if config_format == 'cube_config' and do_logging:
            logger.warning('deprecated `config_format="cube_config"` is used')

        config_obj = config
        if not isinstance(config, StatfaceReportConfig):
            config_obj = StatfaceReportConfig()
            if isinstance(config, dict):
                config_obj.update_from_dict(config)
            elif isinstance(config, (six.binary_type, six.text_type)):
                config_obj.update_from_yaml(config)
            else:
                raise StatfaceClientValueError(
                    'wrong config type: {}'.format(type(config))
                )

        if 'title' in request_data:
            if do_logging:
                logger.warning('deprecated param `title` is used')
            config_obj.title = request_data['title']
        elif title:
            config_obj.title = title
        elif not config_obj.title:
            if do_logging:
                logger.warning('title is not specified')
            config_obj.title = self.path.split('/')[-1]

        config_obj.check_valid()

        config_data = config_obj.to_dict(require_strings=require_strings)

        if config_format == 'top' or require_strings:
            request_data.update(config_data)
        else:
            request_data[config_format] = config_data

        if do_logging:
            request_data_for_logging = request_data.copy()
            for key in UPLOAD_FORMAT_PARAMS.values():
                request_data_for_logging.pop(key, None)
            logger.debug(
                ('upload config with params:\n'
                 '  config_format=%s\n'
                 '  params=%s\n'),
                config_format, request_data_for_logging)

        if request_data:
            request_kwargs['data'] = request_data
        return request_kwargs

    @staticmethod
    def _upload_config(*args, **kwargs):
        raise Exception("This method is private and deprecated")

    def _upload_config_call(self, **request_kwargs):
        return self._report_post('config', **request_kwargs)

    @staticmethod
    def _handle_non_correct_json_response(response, exc, exc_type):  # pylint: disable=invalid-name
        content = response.content
        logger.error('Statface returned non-JSON data: %r', content)
        if response.status_code in UNRETRIABLE_STATUSES:
            message = 'Statface returned non-JSON data, content: {!r}'.format(content)
            raise StatfaceIncorrectResponseError(message, exc)
        if response.status_code in RETRIABLE_STATUSES:
            message = 'Internal error (non-JSON response), content: {!r}'.format(content)
            raise exc_type(message, exc)

    def upload_config(
            self,
            config,
            overwrite=True,
            request_kwargs=None,
            scale=None,
            **statface_params):
        """Change (create) report data config + title.

        :param config: (StatfaceReportConfig, dict, serialized to string yaml)
          report data config.

        optional:
        :param overwrite: (bool) True by default.
          with overwrite=False func raise `StatfaceReportConfigError`
          if report exists on server.

        :param verbose: (1 or 0) add extra info to response, 0 by default.
        :param title: (string) deprecated! text description of report,
         should be set in config.
        """
        request_kwargs = (request_kwargs or {}).copy()
        request_data = (request_kwargs.get('data') or {}).copy()
        request_data.update(statface_params)

        if scale is not None:
            scale = self._normvalidate_scale_param(scale)
            request_data['scale'] = scale

        request_kwargs['data'] = request_data
        request_kwargs = self._process_upload_config_params(
            config=config, request_kwargs=request_kwargs, do_logging=True)

        if not overwrite and not self.client._no_excess_calls:
            if self.check_config_uploaded():
                raise StatfaceReportConfigError(
                    'config is uploaded to server (overwrite denied)')

        # TODO?: catch dimensions changes error
        response = self._upload_config_call(**request_kwargs)

        try:
            return response.json()
        except ValueError as exc:
            self._handle_non_correct_json_response(
                response, exc, exc_type=StatfaceIncorrectResponseError)
            raise

    def _download_data(
            self,
            scale,
            format_title,
            request_kwargs=None,
            **statface_params):
        request_kwargs = request_kwargs or {}
        statface_params['scale'] = scale
        statface_params.setdefault('_fill_missing_dates', 0)

        logger.debug(u'parameters for data downloading:\n%s', statface_params)

        uri = "/_api/statreport/{}/{}".format(format_title, self.path)
        return self.client._request(
            'get', uri, params=statface_params, **request_kwargs)

    def download_data(
            self,
            scale,
            format=PY_DATA_FORMAT,  # pylint: disable=redefined-builtin
            raw=False,
            request_kwargs=None,
            **statface_params):
        """
        Download data from Statface

        Note! Downloads not all of the available data!
        Specify date_min/date_max or _period_distance for proper date interval.
        see also: https://wiki.yandex-team.ru/statbox/Statface/reportdata/

        required:

        :param scale: (string) data scale. One of STATFACE_SCALES

        optional:

        :param format: (string) format of data from Statface server.
          One of STATFACE_DATA_FORMATS.  PY_DATA_FORMAT by default.

        :param raw: (bool) don't parse response to rows. False by default.

        :return: string in `format` if `raw`=True,
          else -- iterator over dicts with parsed rows.

        Examples:

            for record in rpt.download_data(scale=statface_client.constants.MONTHLY_SCALE):
                print(record['fielddate'])

            # CSV parsing.
            # WARNING: should not be used in actual code (use
            # `raw=False` instead).
            csv_data = rpt.download_data(
                scale=statface_client.constants.MONTHLY_SCALE,
                raw=True,
                format=statace_clients.constants.CSV_DATA_FORMAT,
            )
            import csv, StringIO
            records = csv.DictReader(StringIO.StringIO(csv_data), delimiter=';')
            records = (
              {key.decode('cp1251'): value.decode('cp1251') for key, value in record.items()}
              for record in records)
            record = next(records)
            print(record)
        """
        scale = DOWNLOAD_SCALE_PARAMS[scale]
        if format not in STATFACE_DATA_FORMATS:
            raise StatfaceClientValueError('wrong format {}'.format(format))

        if format == PY_DATA_FORMAT:
            raw = False  # the naming is a mess, yes.
            format = JSON_DATA_FORMAT

        if not raw and format == TSKV_DATA_FORMAT:
            logger.warning(
                'TSKV deserialization is deprecated. Please, use JSON.'
            )
            format = JSON_DATA_FORMAT

        _parseable_formats = (JSON_DATA_FORMAT,)
        if not raw and format not in _parseable_formats:
            raise ValueError((
                'no deserialize methods for the format "{}".'
                ' Specify raw=True and use your own parser').format(
                    format))

        report_scale = scale.split('_')[2] if '_by_' in scale else scale
        if report_scale not in self.fetch_scales():
            raise StatfaceClientValueError(
                'report {} has not scale {}'.format(self.path, report_scale))

        response = self._download_data(
            scale,
            format,
            request_kwargs=request_kwargs,
            **statface_params
        )

        if raw:
            return response.content
        try:
            return response.json()['values']
        except (ValueError, KeyError) as exc:
            self._handle_non_correct_json_response(
                response, exc, exc_type=StatfaceClientDataDownloadError)

    @staticmethod
    def _serialize_data(  # pylint: disable=too-many-branches
            data,
            report_config_obj=None, data_format=PY_DATA_FORMAT):

        # Do not serialize the already-serialized data:
        if isinstance(data, (six.binary_type, six.text_type)):
            return data

        if isinstance(data, dict):
            # # TODO?: catch `data={"values": [{...}, ...]}` here?
            # if len(datas) == 1 and 'values' in datas:
            #     if not isinstance(datas['values'], list):
            #         raise ...
            #     datas = datas['values']
            datas = [data]
        elif isinstance(data, list):
            datas = data
        else:
            try:
                datas = list(data)
            except TypeError:
                raise StatfaceClientValueError(
                    'unsupported data type: {}\n\n'
                    '----------====== Original traceback ======----------\n'
                    '{}'.format(
                        type(data), traceback.format_exc()
                    )
                )

        if not datas:
            if data_format == PY_DATA_FORMAT:
                return []
            return ''
        first_record = datas[0]

        if isinstance(first_record, six.text_type):
            # NOTE: `format` compliance is not checked.
            serialized_data = '\n'.join(datas)
        elif isinstance(first_record, six.binary_type):
            # NOTE: `format` compliance is not checked.
            serialized_data = b'\n'.join(datas)
        elif isinstance(first_record, dict):
            if report_config_obj and data_format not in (PY_DATA_FORMAT, JSON_DATA_FORMAT):
                trees = [
                    field for field, field_type in report_config_obj.dimensions.items()
                    if field_type == 'tree'
                ]
                for tree in trees:
                    for record in datas:
                        value = record[tree]
                        if isinstance(value, (list, tuple)):
                            value = [str(x) for x in value]
                            record[tree] = '\t' + '\t'.join(value) + '\t'
            if data_format == PY_DATA_FORMAT:
                serialized_data = datas
            elif data_format == JSON_DATA_FORMAT:
                serialized_data = json.dumps(dict(values=datas))
            else:
                raise NotImplementedError(
                    'no serialization module for {}'.format(data_format)
                )
        else:
            raise StatfaceClientValueError(
                'unsupported data type inside iterator: {}'
                .format(type(first_record))
            )
        return serialized_data

    @classmethod
    def _serialize_data_ext(  # pylint: disable=too-many-branches
            cls, data, report_config_obj=None, data_format=PY_DATA_FORMAT):
        """
        Process data to be uploaded, returning serialized data and various
        information about the passed value and the result.
        """

        data_is_filelike = (
            hasattr(data, 'read') and hasattr(data, 'seek') and
            hasattr(data, 'tell'))
        data_is_serialized = (
            data_is_filelike or
            isinstance(data, (six.binary_type, six.text_type)))

        # When requested to serialize the data into TSKV, warn and serialize
        # into JSON / raw instead.
        if data_format == TSKV_DATA_FORMAT and not data_is_serialized:
            logger.warning('Serializing into TSKV format is deprecated. Please, use JSON.')
            data_format = PY_DATA_FORMAT

        if data_is_serialized:
            serialized_data = data
        else:
            serialized_data = cls._serialize_data(
                data, report_config_obj=report_config_obj,
                data_format=data_format)

        # Attempt to figure out the data size.
        if data_is_filelike:
            serialized_data = data
            _data_start = data.tell()
            data.seek(0, 2)  # move to the end
            serialized_data_size = data.tell()  # get the position
            data.seek(_data_start)  # move to the original starting position
        elif isinstance(data, six.binary_type):
            serialized_data_size = len(data)
        elif isinstance(data, (dict, list)):
            serialized_data_size = None
        else:
            # For `unicode` on py2, this actually results in 2-4 times larger
            # value than the actual utf-8, i.e., essentially, utf-32 size:
            # EMPTY_STR_SIZE = serialized_data.__class__().__sizeof__()
            serialized_data_size = serialized_data.__sizeof__() - EMPTY_STR_SIZE

        return dict(
            serialized_data=serialized_data,
            data_format=data_format,
            filelike=data_is_filelike,
            pre_serialized=data_is_serialized,
            size=serialized_data_size,
        )

    @staticmethod
    def _normvalidate_scale_param(scale):
        result = SCALE_PARAMS.get(scale)
        if result is not None:
            return result
        raise StatfaceClientValueError('unknown scale {}'.format(scale))

    @staticmethod
    def _check_data_format(data_format):
        if data_format not in STATFACE_DATA_FORMATS:
            raise StatfaceClientValueError('unknown data `format` {}'.format(data_format))

    def _process_upload_data_params(  # pylint: disable=too-many-arguments,too-many-locals
            self,
            scale,
            data, data_format=PY_DATA_FORMAT,
            request_kwargs=None,
            auto_multipart=True, report_config_obj=None,
            do_logging=True):
        """
        Convert data and additional parameters to the requests call arguments.

        :param data: the data to be uploaded (either into
        `/_api/report/data/...` or into `/_api/report/config_and_data/...`)

        :param request_kwargs: the full set of keyword arguments for `self._report_post`

        ...
        """
        request_kwargs = (request_kwargs or {}).copy()
        request_data = (request_kwargs.get('data') or {}).copy()
        request_files = (request_kwargs.get('files') or {}).copy()

        scale = self._normvalidate_scale_param(scale)
        request_data['scale'] = scale

        self._check_data_format(data_format)

        data_info = self._serialize_data_ext(
            data, report_config_obj=report_config_obj, data_format=data_format)
        serialized_data = data_info['serialized_data']

        data_format = data_info['data_format']
        format_title = UPLOAD_FORMAT_PARAMS[data_format]

        serialized_data_size = data_info['size']
        if serialized_data_size is not None and do_logging:
            if serialized_data_size > MAX_UPLOAD_BYTES:
                logger.warning(
                    'The size of the sent data (%d) is more than %d bytes',
                    serialized_data_size, MAX_UPLOAD_BYTES)
            else:
                logger.debug('~%d bytes to send', serialized_data_size)

        if data_info['filelike']:
            request_files[format_title] = serialized_data
        elif auto_multipart and isinstance(serialized_data, six.binary_type):
            # This is supposed to result in a 'multipart/form-data' body which
            # can include binary values without additional quoting.
            # TODO?: use this for text type too?
            request_files[format_title] = serialized_data
        else:
            request_data[format_title] = serialized_data

        if request_data:
            request_kwargs['data'] = request_data
        if request_files:
            request_kwargs['files'] = request_files

        if do_logging:
            request_data_for_logging = request_data.copy()
            request_data_for_logging.pop(format_title, None)
            logger.debug(
                ('parameters for data uploading:\n'
                 '  host: %s\n'
                 '  format: %s\n'
                 '  scale: %s\n'
                 ' params: %s\n'),
                self.client.host,
                format_title,
                scale,
                request_data_for_logging,
            )

        return request_kwargs

    @staticmethod
    def _upload_data(*args, **kwargs):
        raise Exception("This method is private and deprecated")

    def _upload_data_call(self, **request_kwargs):
        return self._report_post('data', **request_kwargs)

    def upload_data(
            self,
            scale,
            data,
            format=PY_DATA_FORMAT,  # pylint: disable=redefined-builtin
            request_kwargs=None,
            **statface_params):

        """Upload data to report

        :param scale: (string) (required) one of STATFACE_SCALES

        :param data: (string | iterable of dicts | iterable of strings) (required)
          for unserialized data (iterable of dicts) specify `format`;
          for serialized data (string | iterable of strings), `format` is ignored,
          but in some cases in can be useful.

        optional:
        :param format: (string) one of STATFACE_DATA_FORMATS,
          PY_DATA_FORMAT by default

        :param _append_mode: add data to report scale without rewriting
          old data (0 by default)

        :param replace_mask: list of dimension fields to rewriting overlaps.
        see also:
            https://wiki.yandex-team.ru/statbox/statface/externalreports/#replacemask
        """

        data_format = format

        # Checks before any remote calls:
        self._check_data_format(data_format)
        scale = self._normvalidate_scale_param(scale)

        if self.client._no_excess_calls:
            config = None
        else:
            config = StatfaceReportConfig()
            config.update_from_dict(self.download_config())

        upload_id = statface_params.get('uuid')

        request_kwargs = (request_kwargs or {}).copy()
        request_kwargs['data'] = statface_params.copy()

        request_kwargs = self._process_upload_data_params(
            scale=scale, data=data, data_format=data_format,
            request_kwargs=request_kwargs, report_config_obj=config,
            do_logging=True)

        upload_config = self.client._config['reports']['upload_data']
        if not upload_config['use_upload_id']:
            response = self._upload_data_call(**request_kwargs)
        else:
            with UploadChecker(
                    report_api=self,
                    check_action=upload_config['check_action_in_case_error'],
                    upload_id=upload_id,
            ) as upload_checker:
                request_kwargs.setdefault('data', {})['uuid'] = upload_checker.upload_id
                response = self._upload_data_call(**request_kwargs)

        logger.debug('upload finished correctly')
        return response.json()

    # pylint: disable=too-many-locals
    def upload_yt_data(
            self,
            scale,
            cluster,
            table_path,
            yt_token=None,
            async_mode=False,
            wait_timeout=None,
            request_kwargs=None,
            **statface_kwargs):  # pylint: disable=too-many-arguments

        json_data = statface_kwargs
        json_data.update(dict(
            cluster=cluster,
            table_path=table_path,
        ))
        if yt_token is not None:
            json_data.update(yt_token=yt_token)

        if request_kwargs is None:
            request_kwargs = {}
        request_kwargs.update(dict(
            uri='yt_data_upload',
            json=json_data,
            params=dict(scale=scale),
        ))
        request_kwargs.setdefault('timeout', 120)   # timeout for task scheduling

        response = self._report_post(**request_kwargs)
        logger.info('Successfully scheduled upload task: %s', response.text)
        response_data = response.json()

        if async_mode:
            logger.debug('Async mode - not waiting for the upload result.')
            return response_data

        upload_uuid = response_data.get('uuid')
        checker = UploadChecker(
            report_api=self,
            check_action=ENDLESS_WAIT_FOR_FINISH,
            upload_id=upload_uuid,
            timeout=wait_timeout)
        with checker as checker:
            result = checker.wait_for_finish()
        logger.info('Data upload complete')
        logger.debug('Data upload result info: %r', result)
        return result

    def simple_upload(
            self,
            scale,
            data,
            request_kwargs=None,
            streamed=False,
            **statface_params):
        """
        Special case for uploading (generally) small amounts of data.

        Does **not** support:

          * Transactions.
          * New dimension values: you have to 'update metadata' manually when new values are added.
          * Upload status API.
          * Report history.
          * Scale autocreation: you have to upload some data in a the normal way before this can be used.
          * Dimension uniqueness checks.
          * replace_mask (always rewrites by full dimensions match only, including `fielddate`).
          * Report update events and on-data-upload monitorings.

        See also:
        https://st.yandex-team.ru/STATFACE-5560
        https://wiki.yandex-team.ru/statbox/statface/reportapi/data/simpleupload/
        """
        statface_params['scale'] = scale
        request_kwargs = request_kwargs or {}

        def dump_line(item):
            if not isinstance(item, dict):
                raise ValueError("`data` must be an iterable of dicts")
            line = json.dumps(item) + "\n"
            return line

        body = (dump_line(item) for item in data)

        # Not strictly necessary, but:
        #   * pre-validates data,
        #   * better suits the primary use case,
        #   * makes retries possible.
        if not streamed:
            body = ''.join(body)

        return self._report_post(
            'simple_upload',
            method='put',
            params=statface_params,
            data=body,
            **request_kwargs)

    def _upload_yt_data_wait(self, *args, **kwargs):  # pylint: disable=unused-argument,no-self-use
        raise Exception("Deprecated internal method")

    def update_last_period_distincts(self, scale):
        return self._report_post('update_last_period_distincts', data={'scale': scale}).json()

    def update_cube_state(self, scale):
        return self._report_post('update_cube_state', data={'scale': scale}).json()

    @staticmethod
    def _upload_config_and_data(*args, **kwargs):
        raise Exception("Deprecated internal method")

    def _upload_config_and_data_i(
            self,
            config,
            title,
            data,
            scale,
            data_format=PY_DATA_FORMAT,
            config_format='config',
            request_kwargs=None,
            **statface_params):  # pylint: disable=too-many-arguments
        """
        :param config: user_config data, dict or json or yaml.
        :param data: data to upload

        For data-related parameters, see `upload_data`.
        """
        request_kwargs = (request_kwargs or {}).copy()
        request_data = statface_params.copy()
        request_kwargs['data'] = request_data
        # NOTE: the order is tricky: `_process_upload_data_params` might set
        # the `files` kwargs, which means structures in values aren't allowed,
        # which means the config data from `_process_upload_config_params` has
        # to be a string.
        request_kwargs = self._process_upload_data_params(
            scale=scale, data=data, data_format=data_format,
            request_kwargs=request_kwargs, do_logging=True)
        request_kwargs = self._process_upload_config_params(
            config=config, config_format=config_format, title=title,
            request_kwargs=request_kwargs, do_logging=True)
        # pylint: disable=repeated-keyword
        return self._report_post('config_and_data', **request_kwargs)

    def upload_config_and_data(self, **params):
        params['data_format'] = params.pop('format', PY_DATA_FORMAT)
        with UploadChecker(
                report_api=self,
                check_action=ENDLESS_WAIT_FOR_FINISH,
                upload_id=params.get('uuid')
        ) as upload_checker:
            params['uuid'] = upload_checker.upload_id
            response = self._upload_config_and_data_i(**params)
        logger.debug('upload config and data finished correctly')
        return response.json()

    upload_config_and_data.__doc__ = _upload_config_and_data.__doc__

    def _fetch_data_upload_status(self, upload_id, raise_for_status=True):
        """ Get data upload status response """
        request_uri = '_api/add_data_status/{}'.format(upload_id)
        response = self.client._request(
            'get',
            request_uri,
            raise_for_status=raise_for_status
        )
        return response

    def check_data_upload_status_verbose(self, upload_id):  # pylint: disable=invalid-name
        """
        Get upload status info,

        raise `StatfaceClientDataUploadError` if upload failed.
        """
        response = self._fetch_data_upload_status(upload_id)

        content = response.json()
        try:
            status = content['status']
            if status == 'error':
                default_msg = 'Unknown error. Try to find detailed info in report history.'
                info = (
                    content.get('info') or
                    content.get('message') or
                    content.get('error') or
                    default_msg)
                error_msg = u'data upload failed: {}'.format(info)
                logger.error(error_msg)
                raise StatfaceClientDataUploadError(error_msg)
            elif status == 'aborted':
                raise StatfaceClientDataUploadError("Upload was aborted")
        except KeyError as exc:
            raise StatfaceIncorrectResponseError(exc)
        return content

    def check_data_upload_status(self, upload_id):
        """
        Get upload status,

        raise `StatfaceClientDataUploadError` if upload failed.
        """
        return self.check_data_upload_status_verbose(upload_id)['status']

    def fetch_data_upload_status(self, upload_id):
        """ WARNING: deprecated (unfortunate naming) """
        return self.check_data_upload_status(upload_id)

    def _create_metadata_uri(self):
        path = six.moves.urllib.parse.quote(self.path)
        return '/_api/report_metadata/{}'.format(path)

    def _download_metadata(self):
        request_uri = self._create_metadata_uri()
        response = self.client._request('get', request_uri)
        return response

    def download_metadata(self):
        """Get report metadata"""
        return self._download_metadata().json()

    def _merge_metadata(self, **request_data):
        request_uri = self._create_metadata_uri()
        response = self.client._request(
            'put',
            request_uri,
            data=json.dumps(request_data),
            headers={"content-type": "application/json"}
        )
        return response

    def merge_metadata(self, **request_data):
        """Merge metadata to Statface report metadata.

        Update just selected metadata fields.
        Example:
        report.upload_metadata(
            responsible=['thenno', 'hans'],
            desc_wiki='123',
            my_extra_data={1: 2}
        )
        """
        return self._merge_metadata(**request_data).json()

    def _fetch_missing_dates(self, **params):
        return self._report_get('missing_dates', params=params)

    def fetch_missing_dates(self, scale, **params):
        """
        required:
        :param scale: (string) data scale. One of STATFACE_SCALES

        optional:
        :param date_min: (string), start date in iso format
        :param date_max: (string), end date in iso format
        :param distance_day: (int), number of day in checked time interval
        0 days by default
        :param _missing: (0 or 1 (default for client)),
        1 - return missing dates, 0 - return available by access dates

        :return: dict like
        {
            requested_list: ["2014-03-31"],
            scale: "d",
            missing: ["2014-03-31"],
            requested_interval: ["2014-03-31", "2014-03-31"],
            exists: [ ]
        }

        Examples:
        > report.fetch_missing_dates(scale=MONTHLY_SCALE)
        got missing dates for period
        [current time - one period for scale; current time]
        """
        params['scale'] = SCALE_PARAMS[scale]
        params.setdefault('_missing', 1)
        return self._fetch_missing_dates(**params).json()

    def fetch_available_dates_range(self, scale, **params):
        params['scale'] = SCALE_PARAMS[scale]
        return self._report_get('available_dates', params=params).json()


def do_nothing():
    pass


# TODO: put this class as an attribute on StatfaceReportAPI
class UploadChecker(object):

    status_check_delay = 1.0
    max_cont_check_failures = 10

    # pylint: disable=too-many-arguments
    def __init__(self, report_api, check_action, upload_id, timeout=None, timelimit=None):
        self.api = report_api
        CHECK_ACTIONS = {
            DO_NOTHING: do_nothing,
            ENDLESS_WAIT_FOR_FINISH: self.wait_for_finish,
            JUST_ONE_CHECK: self.check_upload_status
        }
        self.check_action = CHECK_ACTIONS[check_action]
        self.upload_id = upload_id or self.generate_uniq_upload_id()
        if timeout is not None:
            assert timelimit is None, "should not mix timeout and timelimit"
            timelimit = self.now() + timeout
        elif timelimit is not None:
            timeout = timelimit - self.now()  # for exception message, mostly.
        self.timeout = timeout  # max time, duration in seconds.
        self.timelimit = timelimit  # max timestamp, unixtime.

    @staticmethod
    def now():
        # return time.monotonic()
        return time.time()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        if exc_value is None:
            return None  # irrelevant either way
        # http://docs.python-requests.org/en/master/user/quickstart/errors-and-exceptions
        if isinstance(exc_value, requests.exceptions.ConnectionError):
            # A network problem,
            # server didn't get any requests.
            return None  # reraise
        if isinstance(exc_value, requests.exceptions.Timeout):
            # ReadTimeout or ConnectTimeout.
            # Unknown status of uploading, need extra check.
            try:
                self.check_action()
            except requests.exceptions.RequestException as upload_error:
                logger.error('Error during check upload status: %s', upload_error)
                return None  # reraise
            return True  # check succeeded, swallow the exception.
        return None  # reraise whichever else.

    @staticmethod
    def generate_uniq_upload_id():
        return six.text_type(uuid.uuid4())

    def check_upload_status(self):
        """ WARNING: deprecated (bad-for-debug method protocol) """
        return self.api.fetch_data_upload_status(self.upload_id)

    def wait_for_finish(self, status_check_delay=None, max_cont_check_failures=None):
        if status_check_delay is None:
            status_check_delay = self.status_check_delay
        if max_cont_check_failures is None:
            max_cont_check_failures = self.max_cont_check_failures

        cont_failures = 0
        status_data = None
        while True:
            if self.timelimit is not None and self.now() >= self.timelimit:
                raise StatfaceClientDataUploadError(
                    'Data upload timeout ({} seconds)'.format(self.timeout))

            try:
                # status = self.check_upload_status()
                status_data = self.api.check_data_upload_status_verbose(self.upload_id)
                status = status_data['status']
            except StatfaceClientDataUploadError:
                raise
            except Exception as exc:  # pylint: disable=broad-except
                cont_failures += 1
                if cont_failures >= max_cont_check_failures:
                    raise
                logger.warning('Upload status check error: %r', exc)
            else:
                logger.debug('Upload status: %s', status)
                if status == 'success':
                    return status_data
            time.sleep(status_check_delay)
        raise Exception("Programming error: endless loop had an end.")
