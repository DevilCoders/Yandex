# coding=utf-8
import sys
import re2
import os
import cgi
import json
import cPickle as pickle
import traceback
import random
from base64 import urlsafe_b64encode
from time import time, strftime, gmtime
from monotonic import monotonic
from urlparse import urlparse, parse_qsl, parse_qs
from functools import partial
from collections import defaultdict

import cchardet as chardet
import tornado.httpclient
import tornado.web
from tornado.gen import coroutine, TimeoutError
from tornado.http1connection import HTTP1Connection, HTTP1ConnectionParameters
from tornado.simple_httpclient import _HTTPConnection
from tornado.ioloop import IOLoop
from tornado.simple_httpclient import SimpleAsyncHTTPClient

from yabs_filter_record import decode as bk_decode

from antiadblock.libs.decrypt_url.lib import get_key, decrypt_url, SEED_LENGTH, DecryptionError
from antiadblock.cryprox.cryprox.common.cry import generate_seed
import antiadblock.cryprox.cryprox.common.visibility_protection as v_protect
import antiadblock.cryprox.cryprox.service.body_encrypter as body_encrypter
from antiadblock.cryprox.cryprox.common.cryptobody import body_replace, CryptUrlPrefix, body_crypt, \
    crypt_js_body, validate_script_key, crypt_url
from antiadblock.cryprox.cryprox.common.resource_utils import url_to_filename
from antiadblock.cryprox.cryprox.common.tools.misc import duration_usec, update_dict, remove_headers, \
    parse_nonce_from_headers, get_cache_control_max_age
from antiadblock.cryprox.cryprox.common.tools.url import url_add_query_param, url_add_param, UrlClass, \
    url_delete_query_params, classify_url, TAGS_ALLOWED_POST_REQUESTS, url_keep_query_params, \
    url_keep_json_keys_in_query_params
from antiadblock.cryprox.cryprox.common.tools.crypt_cookie_marker import get_crypted_cookie_value
from antiadblock.cryprox.cryprox.common.tools.experiments import is_experiment_date, uid_is_experimental, uid_is_control
from antiadblock.cryprox.cryprox.common.tools.bypass_by_uids import BypassByUids
from antiadblock.cryprox.cryprox.common.tools.internal_experiments import InternalExperiment
from antiadblock.cryprox.cryprox.common.tools.ua_detector import parse_user_agent_data
from antiadblock.cryprox.cryprox.common.tools.ip_utils import check_ip_headers_are_internal, is_local_net
from antiadblock.cryprox.cryprox.common.tools.tornado_retry_request import http_retry
from antiadblock.cryprox.cryprox.config import service as service_config
from antiadblock.cryprox.cryprox.config import system as system_config, bk as bk_config, adfox as adfox_config
from antiadblock.cryprox.cryprox.service.pipeline import EventType
from . import action, decorators
from .decorators import logged_action
from .jslogger import JSLogger
from .resolver import BlacklistingResolver, NO_ADDRESSES_EXCEPTION_MESSAGE
from .metrics import AppMetrics, format_http_host
from antiadblock.encrypter import CookieEncrypter


class DelayedCloseHTTP1Connection(HTTP1Connection):

    def __init__(self, *args, **kwargs):
        super(DelayedCloseHTTP1Connection, self).__init__(*args, **kwargs)
        self.is_local_net = False

    def close(self):
        if self.is_local_net or self.stream.closed() or \
                is_local_net(self.stream.socket, system_config.YANDEX_NETS):
            super(DelayedCloseHTTP1Connection, self).close()
        else:
            IOLoop.current().call_later(5, lambda: super(DelayedCloseHTTP1Connection, self).close())


class DelayedCloseHttpConnection(_HTTPConnection):
    def __init__(self, *args, **kwargs):
        self._connect_time = None
        super(DelayedCloseHttpConnection, self).__init__(*args, **kwargs)

    def _create_connection(self, stream):
        stream.set_nodelay(True)
        connection = DelayedCloseHTTP1Connection(
            stream, True,
            HTTP1ConnectionParameters(
                no_keep_alive=True,
                max_header_size=self.max_header_size,
                max_body_size=self.max_body_size,
                decompress=self.request.decompress_response),
            self._sockaddr)
        return connection

    def _on_connect(self, stream):
        self._connect_time = self.io_loop.time() - self.start_time
        super(DelayedCloseHttpConnection, self)._on_connect(stream)

    def _run_callback(self, response):
        if self._connect_time is not None:
            response.time_info['connect_time'] = self._connect_time
        super(DelayedCloseHttpConnection, self)._run_callback(response)

    def _on_end_request(self):
        is_local = is_local_net(self.stream.socket, system_config.YANDEX_NETS)
        self.connection.is_local_net = is_local
        if is_local:
            super(DelayedCloseHttpConnection, self)._on_end_request()
        else:
            IOLoop.current().call_later(5, lambda: super(DelayedCloseHttpConnection, self)._on_end_request())


class DelayedCloseAsyncHttpClient(SimpleAsyncHTTPClient):

    def _connection_class(self):
        return DelayedCloseHttpConnection


def configure_http_client(configs):
    # we merge PROXY_CLIENT_CONFIG from all the configs to one_big_proxy_config
    # thus intersecting values may shoot your ass
    one_big_proxy_config = service_config.PROXY_CLIENT_CONFIG
    for config in configs.values():
        one_big_proxy_config = update_dict(config.PROXY_CLIENT_CONFIG, one_big_proxy_config)
    tornado.httpclient.AsyncHTTPClient.configure(DelayedCloseAsyncHttpClient, resolver=None, **one_big_proxy_config)


class HttpHandler(tornado.web.RequestHandler):
    """
    Принимает шифрованные урлы дешифрует и делает accel-redir для картинок для остального запрашивает контент
    """
    SUPPORTED_METHODS = ['GET', 'POST']

    # noinspection PyMethodOverriding
    def initialize(self, config, **kwargs):
        self.pipeline = kwargs.get('pipeline')
        self.config = config
        self.request_id = self.request.headers.get(system_config.REQUEST_ID_HEADER_NAME, 'None')
        self.request.tags = []
        self.original_url = urlparse(self.request.full_url())
        self.request.url = self.original_url
        self.is_crypted_url = None
        self.logger = JSLogger('root', request_id=self.request_id, service_id=config.name, cfg_version=str(config.version), request_tags=self.request.tags)
        self.metrics = AppMetrics(service_id=config.name)

        self.request.user_agent_data = kwargs.get("ua_data", parse_user_agent_data(self.request.headers.get('User-Agent')))
        self.request.bypass = False

        self.logger.setLevel(config.logging_level)
        self.start_time_for_pipeline = time()
        self.start_time = monotonic()
        self.full_action = action.HANDLER_NONE
        self.last_response = None
        self.cookie_encrypter = CookieEncrypter(service_config.COOKIE_ENCRYPTER_KEYS_PATH)
        self.meta_callback = None
        self.rtb_auction_via_script = config.RTB_AUCTION_VIA_SCRIPT
        self.rtb_jsonp_response = False
        self.crypted_host = self.request.headers.get(system_config.CRYPTED_HOST_HEADER_NAME) or config.CRYPTED_HOST or self.request.host

        self.prefix = random.choice(self.config.CRYPT_URL_RANDOM_PREFFIXES) if self.config.CRYPT_URL_RANDOM_PREFFIXES else self.config.CRYPT_URL_PREFFIX

        # TODO: Remove after https://st.yandex-team.ru/ANTIADB-1783
        self.header_name = None
        self.hidden_args = None
        self.hidden_args_decoded = None

        # https://st.yandex-team.ru/ANTIADB-2224
        self.response_logger = None
        self.request.loggable = int(self.request.headers.get(system_config.FULL_RESPONSE_LOGGABLE_HEADER, '0'))
        if self.request.loggable:
            self.response_logger = JSLogger('ResponseLogger', log_file=service_config.RESPONSE_LOG_FILE, request_id=self.request_id,
                                            service_id=config.name, request_tags=self.request.tags)

        # https://st.yandex-team.ru/ANTIADB-2402
        self.original_netloc = None
        self.end2end_logger = None
        if service_config.END2END_TESTING:
            self.end2end_logger = JSLogger('End2EndLogger', log_file=service_config.END2END_TESTING_LOG_FILE, request_id=self.request_id,
                                           service_id=config.name, request_tags=self.request.tags)

        self.is_argus_handler = False
        self.checked_ip_headers = kwargs.get("checked_ip_headers", {})

    def get_uid(self):
        yandexuid_cookie = self.request.cookies.get(system_config.YAUID_COOKIE_NAME, None)
        if yandexuid_cookie is not None:
            return yandexuid_cookie.value
        encrypted_cookie_value = self.get_crypted_yauid_cookie_value()
        if encrypted_cookie_value:
            return self.cookie_encrypter.decrypt_cookie(encrypted_cookie_value)

    def finish(self, chunk=None):
        self._headers[system_config.NGINX_SERVICE_ID_HEADER] = self.config.name  # https://st.yandex-team.ru/ANTIADB-2013
        if self.end2end_logger is not None:
            self.end2end_logger.info('', body=''.join(self._write_buffer), status_code=self.get_status(),
                                     request_url=self.request.url.geturl())
        super(HttpHandler, self).finish(chunk)

    def compute_etag(self):
        return None  # disable tornado Etag

    def check_etag_header(self):
        return False

    def _get_cache_expire(self, cache_control):
        return min(15 * 60, get_cache_control_max_age(cache_control))

    def init_binurlprefix_with_crypt_key(self, crypted_host, seed, headers):
        scheme, scheme_params = cgi.parse_header(headers.get('X-Forwarded-Proto', 'http'))
        self.binurlprefix = CryptUrlPrefix(scheme, crypted_host, seed, prefix=self.prefix)
        self.crypt_key = get_key(self.config.CRYPT_SECRET_KEY, seed)

    # обработка body полученного на request от запроса прокси в методе fetch_url
    def process_response(self, seed, response, http_client, cache_callback=None):
        # http_client.close() # DO NOT close connection explicitly, see ANTIADB-1767
        config = self.config
        request = self.request
        connect_time = int(response.time_info.get('connect_time', 0) * 10 ** 6)
        self.last_response = response
        tags_string = ','.join(sorted([tag.name for tag in request.tags]))
        host_for_metrics = format_http_host(request.url.hostname) if not config.INTERNAL and UrlClass.PARTNER in request.tags else request.url.hostname

        if connect_time:
            self.logger.debug(None, action=action.FETCH_CONTENT_CONNECT_TIME, duration=connect_time)
            self.metrics.increase_histogram_by_dimensions(action.FETCH_CONTENT_CONNECT_TIME, connect_time, http_host=host_for_metrics)

        # TODO: Functional tests for 403 and other things
        if response.code == 599:  # timeout
            if isinstance(response.error, IOError) and response.error.message == NO_ADDRESSES_EXCEPTION_MESSAGE:
                self.set_status(status_code=403, reason=response.error.message)
            else:
                duration = int(response.request_time * 10 ** 6)  # in micro seconds

                if duration <= config.NETWORK_FAILS_RETRY_THRESHOLD * 1000:
                    self.set_status(598, 'Retryable Timeout')
                else:
                    self.set_status(response.code, 'Timeout')

                self.logger.info(None,
                                 action=action.FETCH_CONTENT,
                                 duration=duration,
                                 http_host=request.url.hostname,
                                 http_code=response.code,
                                 human_readable_reason=response.reason,
                                 exception=str(response.error),
                                 url=response.effective_url)

                self.metrics.increase_counter(action.FETCH_CONTENT, http_host=host_for_metrics, http_code=response.code)
                self.metrics.increase_histogram_by_dimensions(action.FETCH_CONTENT, duration, http_host=host_for_metrics, tag=tags_string)
            self.finish()
            duration = int((monotonic() - self.start_time) * 10 ** 6)
            self.logger.info(None, action=self.full_action, http_host=request.url.hostname, duration=duration)
            self.metrics.increase_counter(self.full_action, http_host=host_for_metrics)
            self.metrics.increase_histogram_by_dimensions(self.full_action, duration, http_host=host_for_metrics, tag=tags_string)
            return
        if response.error and not isinstance(response.error, tornado.httpclient.HTTPError):
            duration = int((monotonic() - self.start_time) * 10 ** 6)
            self.logger.info(None, action=self.full_action, http_host=request.url.hostname, duration=duration)

            self.metrics.increase_counter(self.full_action, http_host=host_for_metrics)
            self.metrics.increase_histogram_by_dimensions(self.full_action, duration, http_host=host_for_metrics, tag=tags_string)
            raise response.error
        try:
            content_length = len(response.body) if response.body else 0
            if self.response_logger is not None:
                uid = self.get_uid()
                self.response_logger.info('',
                                          status_code=pickle.dumps((response.code, response.reason)),
                                          response=pickle.dumps((response.body, response.headers)),
                                          duration=int(response.request_time * 10 ** 6),
                                          uid=uid,
                                          body_size=content_length)
            self.set_status(response.code, response.reason)
            self._headers = response.headers

            if 300 < response.code < 400:
                self.process_location_header(seed)

            remove_headers(self._headers, config.deny_headers_backward)
            if UrlClass.YANDEX in request.tags:
                remove_headers(self._headers, config.deny_headers_backward_proxy)
                if UrlClass.BK in request.tags:
                    remove_headers(self._headers, config.deny_headers_backward_bk)

            self.content_type, content_params = cgi.parse_header(response.headers.get('Content-Type', ''))

            # fix invalid content-type for js and css
            # https://st.yandex-team.ru/ANTIADB-1788
            file_extention = os.path.splitext(self.request.url.path)[1]
            fix_mime_type = self.content_type in system_config.CONTENT_TYPES_FOR_FIX and file_extention in system_config.FIXED_CONTENT_TYPE
            if fix_mime_type:
                self.content_type = system_config.FIXED_CONTENT_TYPE[file_extention]
                self._headers['Content-Type'] = system_config.FIXED_CONTENT_TYPE[file_extention]

            duration = int(response.request_time * 10 ** 6)
            self.logger.info(None,
                             action=action.FETCH_CONTENT,
                             duration=duration,
                             http_host=request.url.hostname,
                             http_code=response.code,
                             content_type=self.content_type + ('[fixed]' if fix_mime_type else ''),
                             content_length=content_length,
                             url=response.effective_url)

            self.metrics.increase_counter(action.FETCH_CONTENT, http_host=host_for_metrics, http_code=response.code)
            self.metrics.increase_histogram_by_dimensions(action.FETCH_CONTENT, duration, http_host=host_for_metrics, tag=tags_string)
            fetch_content_duration = duration

            to_cache = None
            if response.body:
                if self.request.bypass or response.code not in (200, 404):
                    crypted_body = response.body
                else:
                    crypted_body = bytes(response.body)
                    # TODO: fix after https://st.yandex-team.ru/NANPU-867
                    if config.crypt_mime_re.match(self.content_type) or (not self.content_type and UrlClass.NANPU in request.tags):
                        crypted_host = self.crypted_host
                        if config.name in system_config.CRYPTED_HOST_OVERRIDE_SERVICE_WHITELIST and \
                           response.headers.get(system_config.CRYPTED_HOST_OVERRIDE_HEADER_NAME, '') != '':  # ANTIADB-2798
                            crypted_host = response.headers.get(system_config.CRYPTED_HOST_OVERRIDE_HEADER_NAME, '')
                        self.init_binurlprefix_with_crypt_key(crypted_host, seed, request.headers)

                        if config.CRYPT_LINK_HEADER and 'link' in response.headers:
                            self.crypt_link_header(response.headers)

                        crypted_body = self.crypt_response(seed, crypted_body, content_params)
                    if cache_callback is not None and response.code == 200:
                        to_cache = crypted_body
                self.write(crypted_body)

            self._update_headers(request)
            if to_cache is not None:
                cache_control = response.headers.get('Cache-Control', '')
                cache_expire = self._get_cache_expire(cache_control)
                if cache_expire > 0:
                    cache_callback(crypted_body=to_cache, headers=self._headers, seconds=cache_expire)

            self.finish()
            duration = int((monotonic() - self.start_time) * 10 ** 6)
            self.logger.info(None, action=self.full_action, http_host=request.url.hostname, duration=duration)
            self.metrics.increase_counter(self.full_action, http_host=host_for_metrics)
            self.metrics.increase_histogram_by_dimensions(self.full_action, duration, http_host=host_for_metrics, tag=tags_string, content_type=self.content_type)
            # https://st.yandex-team.ru/ANTIADB-3112
            self.logger.info(None, action=action.PROCESS_BODY, http_host=request.url.hostname, duration=duration)
            self.metrics.increase_histogram_by_dimensions(action.PROCESS_BODY, duration - fetch_content_duration, http_host=host_for_metrics)
        except tornado.httpclient.HTTPError:
            raise
        except Exception:
            self.logger.exception("Exception in handle request")
            raise

    def process_location_header(self, seed):
        location = self._headers.get('Location')
        if location is not None:
            try:
                url = location.decode('utf-8').encode('iso-8859-1')
                config = self.config
                patched_url = "\"{}\"".format(url)
                if config.crypt_url_re.match(patched_url) is not None:
                    self.init_binurlprefix_with_crypt_key(self.crypted_host, seed, self.request.headers)
                    crypted_url = body_crypt(body=patched_url,
                                             binurlprefix=self.binurlprefix,
                                             key=self.crypt_key,
                                             crypt_url_re=config.crypt_url_re,
                                             enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                                             min_length=config.RAW_URL_MIN_LENGTH,
                                             crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                                             config=config,
                                             partner_cookieless_url_re_match=config.partner_to_cookieless_host_urls_re_match,
                                             bypass_url_re_match=config.bypass_url_re_match)
                    url = crypted_url[1:-1]
                self._headers.update({"Location": url})
            except:
                pass

    def crypt_link_header(self, headers):
        config = self.config
        crypted_link = body_crypt(body=headers.get('link'),
                                  binurlprefix=self.binurlprefix,
                                  key=self.crypt_key,
                                  crypt_url_re=config.crypt_link_header_re,
                                  enable_trailing_slash=config.CRYPT_ENABLE_TRAILING_SLASH,
                                  min_length=config.RAW_URL_MIN_LENGTH,
                                  crypted_url_mixing_template=config.CRYPTED_URL_MIXING_TEMPLATE,
                                  partner_url_re_match=config.partner_url_re_match,
                                  config=config,
                                  partner_cookieless_url_re_match=config.partner_to_cookieless_host_urls_re_match,
                                  bypass_url_re_match=config.bypass_url_re_match,
                                  )
        headers.update({'link': crypted_link})

    def crypt_response(self, seed, response_body, content_params):
        # response_body необходимо передавать отдельным параметром
        # так как, если зафейлился запрос за скриптом детекта, то содержимое забирается не из response.body, а из файла с кешом
        url = self.last_response.effective_url
        # restore original netloc for crypt relative urls
        if service_config.END2END_TESTING and self.original_netloc is not None:
            url = urlparse(url)._replace(netloc=self.original_netloc).geturl()
        headers = self.last_response.headers
        return self.crypt_body(seed, response_body, content_params, url, headers)

    def crypt_body(self, seed, body, content_params, url, headers):
        config = self.config
        request = self.request

        def log_metric(action_type, names):
            duration = sum(parsed_body.duration(name) for name in names)
            self.logger.info(None, action=action_type, duration=duration)
            self.metrics.increase(action_type, duration)

        # Need crypt_body_with_tags_re because one cannot just copy crypt_body_re and then update it
        # The reason is that re2.Pattern objects are not copying properly
        # See https://stackoverflow.com/questions/51544483/how-can-i-copy-pyre2-pattern-object
        replace_body_re = config.replace_body_with_tags_re if UrlClass.STATICMON not in request.tags else config.replace_body_re
        if self.is_argus_handler:
            replace_body_re[re2.compile(r"\(\d+,\w+\(\d+\)\.getRandomInt\)\(1e12,2e12\)")] = \
                '(function(){return Math.floor((window.myArray = (window.myArray + 0.001 || 0.0)) * 1000000)})()'

        nonce_js, nonce_css = parse_nonce_from_headers(headers)
        context = body_encrypter.EncryptionContext(
            http_handler=self,
            config=config,
            content_type=self.content_type,
            charset=content_params.get('charset', None),
            key=self.crypt_key,
            request=request,
            url=url,
            seed=seed,
            metrics=self.metrics,
            binurlprefix=self.binurlprefix,
            nonce_js=nonce_js,
            meta_callback=self.meta_callback,
            rtb_auction_via_script=self.rtb_auction_via_script,
            rtb_jsonp_response=self.rtb_jsonp_response,
            nonce_css=nonce_css,
            replace_body_re=replace_body_re,
            headers=headers,
            logger=self.logger,
            last_response_successfull=self.last_response and self.last_response.code == 200,
            is_argus_handler=self.is_argus_handler,
        )
        parsed_body = body_encrypter.parse_body(body, context)
        parsed_body.execute_step(body_encrypter.html_js_body_replace_step, 'process_tags')
        parsed_body.execute_step(body_encrypter.partner_replace_step, 'replace')
        parsed_body.execute_step(body_encrypter.replace_resource_with_xhr_step)

        # Crypt relative urls
        self._process_relative_urls(config, parsed_body, request)

        parsed_body.execute_step(body_encrypter.body_crypt_main_step, 'body_crypt')
        log_metric(action.BODYCRYPT, ['body_crypt', 'relative', 'process_tags'])

        parsed_body.execute_step(body_encrypter.process_rtb_auction_response_step)
        parsed_body.execute_step(body_encrypter.process_nanpu_auction_response_step)
        parsed_body.execute_step(body_encrypter.process_body_replace_step, 'replace')
        if parsed_body.duration_exists('replace'):
            log_metric(action.BODYREPLACE, ['replace'])

        parsed_body.execute_step(body_encrypter.adb_function_replace_step)
        parsed_body.execute_step(body_encrypter.adb_cookies_replace_step)

        self._replace_pcode_placeholders(log_metric, parsed_body)

        parsed_body.execute_step(body_encrypter.add_second_domain_to_csp_step)
        parsed_body.execute_step(body_encrypter.add_patch_to_csp_step)
        return parsed_body.build_content()

    def _replace_pcode_placeholders(self, log_metric, parsed_body):
        # Replace some placeholders in PCODE js scripts to provide correct detect and crypting methods
        if self.content_type in ['text/javascript', 'application/javascript', 'application/x-javascript']:
            parsed_body.execute_step(body_encrypter.autoredirect_script_step, 'autoredirect')
            if parsed_body.duration_exists('autoredirect'):
                log_metric(action.INJECT_AUTOREDIRECT_SCRIPT, ['autoredirect'])
            parsed_body.execute_step(body_encrypter.js_replace_step)
            parsed_body.execute_step(body_encrypter.crypt_js_links_step)
        else:
            parsed_body.execute_step(body_encrypter.crypt_inline_js_step)
            parsed_body.execute_step(body_encrypter.add_nonce_step)

    @staticmethod
    def _process_relative_urls(config, parsed_body, request):
        # RAMBLER relative urls
        if config.encryption_step_enabled(system_config.EncryptionSteps.advertising_crypt) \
                and UrlClass.RAMBLER in request.tags:
            parsed_body.execute_step(body_encrypter.crypt_rambler_relative_urls_step, 'relative')
        # Partner relative urls
        elif UrlClass.PARTNER in request.tags and config.partner_crypt_relative_url_re is not None:
            parsed_body.execute_step(body_encrypter.crypt_partner_relative_urls_step, 'relative')
        # Canvas HTML5 banners
        # относительные урлы есть не только в файлах с полем basePath, но и в произвольных js-ках,
        # например, https://storage.mds.yandex.net/get-canvas-html5/928316/154bc265-9a76-413f-b448-1925237be39b/300x300_canvas.js
        # https://st.yandex-team.ru/ANTIADB-1788#5d37fe68da275f001f39aceb
        elif config.encryption_step_enabled(system_config.EncryptionSteps.advertising_crypt) \
                and UrlClass.YANDEX in request.tags:
            parsed_body.execute_step(body_encrypter.crypt_yandex_canvas_ads_relative_urls_step, 'relative')

    def request_append_extuid(self, extuid_tag_param, extuid_param, extuid_tag, extuid_value):
        if extuid_tag_param and extuid_tag:
            self.request.url = url_add_query_param(self.request.url, extuid_tag_param, extuid_tag, True)
        if extuid_param and extuid_value:
            self.request.url = url_add_query_param(self.request.url, extuid_param, extuid_value, True)

    def check_ip_headers(self):
        if not self.checked_ip_headers:
            self.checked_ip_headers = check_ip_headers_are_internal(system_config.YANDEX_NETS, self.request.headers)
        self.metrics.increase_counter(action=action.CHECK_IP_HEADERS,
                                      tag=','.join(sorted([tag.name for tag in self.request.tags])),
                                      **self.checked_ip_headers)

    def check_uid_cookies(self):
        """
            Check that at least one of uid cookies is present in request
        """
        is_uid_cookie_present = any([self.request.cookies.get(cookie_name) is not None for cookie_name in
                                     self.config.EXTUID_COOKIE_NAMES + ['yandexuid', 'i', 'crookie']])
        self.metrics.increase_counter(action=action.CHECK_UID_COOKIES,
                                      tag=','.join(sorted([tag.name for tag in self.request.tags])),
                                      uid_cookie_present=int(is_uid_cookie_present))

    def _update_headers(self, request):
        for url_re, update_headers_values_dict in self.config.UPDATE_RESPONSE_HEADERS_VALUES.iteritems():
            if url_re.match(request.url.geturl()):
                self._headers.update(update_headers_values_dict)

    def fetch_url(self, seed, cache_callback=None):
        config = self.config
        client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
        # We need to check if external partners are trying to use allowed ip addresses
        if config.INTERNAL is False and (UrlClass.PARTNER in self.request.tags or UrlClass.AUTOREDIRECT_SCRIPT in self.request.tags) and UrlClass.YANDEX_METRIKA not in self.request.tags:
            client.tcp_client.resolver = BlacklistingResolver(resolver=client.tcp_client.resolver, blacklisted_networks=config.YANDEX_NETS, request_id=self.request_id, service_id=self.config.name)
        self.request.headers[system_config.PROXY_SERVICE_HEADER_NAME] = "1"
        remove_headers(self.request.headers, config.deny_headers_forward)
        # https://st.yandex-team.ru/ANTIADB-2295 update request headers per url
        for url_re, header_values in self.config.UPDATE_REQUEST_HEADER_VALUES.iteritems():
            if url_re.match(self.request.url.geturl()):
                self.request.headers.update(header_values)
        # https://st.yandex-team.ru/ANTIADB-3105
        if UrlClass.STRM in self.request.tags and system_config.STRM_TOKEN_HEADER_NAME not in self.request.headers and service_config.STRM_TOKEN:
            self.request.headers[system_config.STRM_TOKEN_HEADER_NAME] = service_config.STRM_TOKEN
        redirect_enabled = config.follow_redirect_url_re_match is not None and config.follow_redirect_url_re_match.match(bytes(self.request.url.geturl())) is not None
        # Replace url part for internal L7 partners to prevent proxying to common L7 slb
        # https://st.yandex-team.ru/ANTIADB-1038
        if self.config.INTERNAL and config.service_slb_url_re_match is not None and self.original_url.hostname != config.second_domain and \
                config.service_slb_url_re_match.match(self.request.url.geturl()) and self.request.headers.get('X-Forwarded-For'):
            # Add header Host to save its value cause there is no header Host when fetching decrypted url by default
            self.request.headers['Host'] = self.request.url.hostname
            # Use last IP from X-Forwarded-For header cause its a partner server IP
            service_slb_host = self.request.headers.get('X-Forwarded-For').rsplit(',', 1)[-1].strip()
            service_slb_port = self.request.headers.get('X-Yandex-Service-L7-Port', '80')
            # Если пришел v6 адрес, не забываем обернуть его в квадратные скобки
            service_slb_host = '[{}]:{}'.format(service_slb_host, service_slb_port) if service_slb_host.find(':') > 0 \
                else '{}:{}'.format(service_slb_host, service_slb_port)
            # Сервисные L7 балансеры работают только по HTTP, HTTPS же терминируется на общем L7
            self.request.url = self.request.url._replace(netloc=service_slb_host, scheme='http')
        # for load_testing: rewrite all request to stub, but save original netloc for crypt relative urls
        if service_config.ENV_TYPE == "load_testing":
            self.original_netloc = self.request.url.netloc
            self.request.url = self.request.url._replace(netloc="{}:{}".format(service_config.STUB_SERVER_HOST, service_config.STUB_SERVER_PORT),
                                                         scheme='http')

        client_request = tornado.httpclient.HTTPRequest(url=self.request.url.geturl(), method=self.request.method,
                                                        body=self.request.body, headers=self.request.headers,
                                                        follow_redirects=redirect_enabled,
                                                        **config.PROXY_CLIENT_REQUEST_CONFIG)

        self.logger.debug(None, message='fetch_url', url=client_request.url)
        client.fetch(client_request, lambda response: self.process_response(seed, response, client, cache_callback), raise_error=False)

    def get_extuid_cookie_value(self):
        """
        Check cookies for exists one of current partner EXTUID_COOKIE_NAMES and return value of first of them,
        or None if no one extuid cookie find
        """
        for cookie_name in self.config.EXTUID_COOKIE_NAMES:
            extuid_cookie_value = self.request.cookies.get(cookie_name, None)
            if extuid_cookie_value:
                return extuid_cookie_value.value
        return None

    def get_crypted_yauid_cookie_value(self):
        """
        Check cookies for exists CRYPTED_YAUID_COOKIE_NAME and return its value,
        or None if no one cookie found
        """
        crypted_yuid_cookie = self.request.cookies.get(system_config.CRYPTED_YAUID_COOKIE_NAME, None)
        if crypted_yuid_cookie is not None:
            return crypted_yuid_cookie.value
        return None

    def yandex_ads_fetch_crypted_yandexuid(self, seed):
        """
        Вызывается, если пришла криптованная yandexuid кука. Кука расшифровывается и шлется обычный запрос с yandexuid-ом
        """

        def process_decryption_failure():
            self.metrics.increase_counter('decrypting_yauid', success=0)
            self.yandex_ads_fetch_extuid(seed, self.get_extuid_cookie_value())

        encrypted_cookie = self.get_crypted_yauid_cookie_value()
        if encrypted_cookie:
            decrypted_cookie = self.cookie_encrypter.decrypt_cookie(encrypted_cookie)
            if decrypted_cookie:
                # Куки приходится доклеивать через хедер, а не через request.cookies, так как request.cookies - это специальный объект для чтения кук, а не просто словарик T_T
                self.request.headers['Cookie'] += '; {}={}'.format(system_config.YAUID_COOKIE_NAME, decrypted_cookie)
                self.metrics.increase_counter('decrypting_yauid', success=1)
                self.yandex_ads_fetch_yandexuid(seed)
            else:
                self.logger.warning('Failed to decrypt cookie: {}'.format(encrypted_cookie))
                process_decryption_failure()
        else:
            self.logger.warning('Encrypted cookie is empty')
            process_decryption_failure()

    def yandex_ads_fetch_extuid(self, seed, extuid_value=None):
        """
            Вызывается, если пришла кукиматчинговая кука.
            В урлы Аукциона и Счетчики надо слать adb_enabled И ext-uid параметры.
            В урлы Счетчиков НЕ нужно слать лишние query string параметры. Счетчики часто отправляюся рекламодателям с
            этими параметрами, рекламодатель может сломаться. У БК есть список аргументов что они отрезают при отправке рекламодателям,
            можно внести эти параметры в их список, или просто не слать тут.
        """
        if UrlClass.ADFOX in self.request.tags:
            self.request_append_extuid(adfox_config.ADFOX_EXTUID_TAG_PARAM, adfox_config.ADFOX_EXTUID_PARAM, self.config.EXTUID_TAG, extuid_value)
            self.add_adblock_scheme_params()
        elif UrlClass.BK in self.request.tags:
            self.request_append_extuid(bk_config.BK_EXTUID_TAG_PARAM, bk_config.BK_EXTUID_PARAM, self.config.EXTUID_TAG, extuid_value)
            self.add_adblock_scheme_params()
        self.fetch_url(seed)

    def yandex_ads_fetch_yandexuid(self, seed):
        """
            Вызывается если пришла кука yandexuid
            В урлы Аукциона и Счетчики надо слать adb_enabled И ext-uid параметры.
            В урлы Счетчиков НЕ нужно слать лишние query string параметры. Счетчики часто отправляюся рекламодателям с
            этими параметрами, рекламодатель может сломаться. У БК есть список аргументов что они отрезают при отправке рекламодателям,
            можно внести эти параметры в их список, или просто не слать тут.
        """
        # Удаляем квери-арги с партнерским тэгом и extid, так как они могли быть добавлены на клиентской стороне: https://st.yandex-team.ru/ANTIADB-350
        self.request.url = url_delete_query_params(self.request.url, [adfox_config.ADFOX_EXTUID_PARAM,
                                                                      adfox_config.ADFOX_EXTUID_TAG_PARAM,
                                                                      adfox_config.ADFOX_EXTUID_LOADER_PARAM,
                                                                      adfox_config.ADFOX_EXTUID_TAG_LOADER_PARAM,
                                                                      bk_config.BK_EXTUID_TAG_PARAM,
                                                                      bk_config.BK_EXTUID_PARAM,
                                                                      bk_config.JSONP_PARAM])

        if UrlClass.BK in self.request.tags or UrlClass.ADFOX in self.request.tags:
            self.add_adblock_scheme_params()
        self.fetch_url(seed)

    def append_hidden_args(self, url):
        """
        1. https://st.yandex-team.ru/ANTIADB-1671
        Header should match config.HIDE_URI_PATH_HEADER_RE regexp. Then we expect this to be a header we were waiting for
        Header value expected to be a string (part of url without domain)
        2. https://st.yandex-team.ru/ANTIADB-2883
        Header should match config.HIDE_GRAB_HEADER_RE regexp. Then we expect this to be a header we were waiting for
        Header value expected to be a string (value of grab)
        """
        possible_path_headers = []
        possible_grab_headers = []
        for h_name, h_value in self.request.headers.items():
            if self.config.hide_uri_path_re.match(h_name):
                possible_path_headers.append((h_name, h_value))
            elif self.config.hide_grab_re.match(h_name):
                possible_grab_headers.append((h_name, h_value))

        header_path_value = ""
        header_grab_value = ""

        if possible_path_headers:
            if len(possible_path_headers) > 1:
                self.logger.warning("Multiple hidden path headers", header_names=[h[0] for h in possible_path_headers])
            self.logger.debug("Got hidden path headers", headers=possible_path_headers)

            header_path_name, header_path_value = possible_path_headers[0]
            del self.request.headers[header_path_name]

        if possible_grab_headers:
            if len(possible_grab_headers) > 1:
                self.logger.warning("Multiple hidden grab value", header_names=[h[0] for h in possible_grab_headers])
            self.logger.debug("Got hidden grab value", headers=possible_grab_headers)

            header_grab_name, header_grab_value = possible_grab_headers[0]
            del self.request.headers[header_grab_name]

        if header_path_value:
            if header_grab_value:
                separator = "&" if "?" in header_path_value or "?" in url else "?"
                appended_part = header_path_value + separator + "grab=" + header_grab_value
            else:
                appended_part = header_path_value
            if url[-1] == "/" and appended_part[0] == "/":
                url += appended_part[1:]
            else:
                url += appended_part

        return url

    def add_adb_bits(self):
        adb_bits_value = int(parse_qs(self.request.url.query).get('adb-bits', [1])[0])
        self.request.url = url_add_query_param(self.request.url, 'adb-bits', adb_bits_value | 1, overwrite=True)

    def add_adblock_scheme_params(self):

        def disabled_tga_with_creatives():
            if not self.config.DISABLE_TGA_WITH_CREATIVES:
                return
            tga_with_creatives = parse_qs(self.request.url.query).get(bk_config.BK_TGA_WITH_CREATIVES_PARAM, [''])[-1]
            if not tga_with_creatives:
                return
            self.request.url = url_add_query_param(self.request.url, bk_config.BK_TGA_WITH_CREATIVES_PARAM, 0, overwrite=True)

        def add_aim_banner_id():
            if self.config.AIM_BANNER_ID_DEBUG_VALUE is None:
                return
            self.request.url = url_add_query_param(self.request.url, bk_config.BK_AIM_BANNER_ID_PARAM, self.config.AIM_BANNER_ID_DEBUG_VALUE, overwrite=True)

        def add_disabled_ad_types():
            if not self.config.DISABLED_BK_AD_TYPES:
                return
            bk_disadbled_ad_types = parse_qs(self.request.url.query).get(bk_config.BK_DISABLE_AD_PARAM, [''])[-1]
            bk_disadbled_ad_types = int(bk_disadbled_ad_types) if bk_disadbled_ad_types.isdigit() else 0
            self.request.url = url_add_query_param(self.request.url, bk_config.BK_DISABLE_AD_PARAM,
                                                   bk_disadbled_ad_types | self.config.DISABLED_BK_AD_TYPES,
                                                   overwrite=True)

        def add_ad_query_args():
            self.add_adb_bits()
            add_disabled_ad_types()
            disabled_tga_with_creatives()

        if UrlClass.ADFOX in self.request.tags:
            if UrlClass.RTB_COUNT not in self.request.tags:
                if UrlClass.BK in self.request.tags:
                    self.request.url = url_add_param(self.request.url, bk_config.BK_URL_PARAMS)
                add_ad_query_args()
            if UrlClass.BK not in self.request.tags:
                self.request.headers['X-Adfox-S2s-Key'] = adfox_config.ADFOX_S2SKEY
            if self.config.ADFOX_DEBUG:
                self.request.headers['X-Adfox-Debug'] = '1'

        elif UrlClass.BK in self.request.tags:
            self.request.url = url_add_param(self.request.url, bk_config.BK_URL_PARAMS)
            if UrlClass.BK_VMAP_AUCTION in self.request.tags:
                self.add_adb_bits()
            elif UrlClass.RTB_COUNT not in self.request.tags:
                add_ad_query_args()
                add_aim_banner_id()
            else:
                # https://st.yandex-team.ru/ANTIADB-1069 [Директ на Морде] подставляем test-tag в счетчики на бэкенде
                test_tag_value = int(parse_qs(self.request.url.query).get('test-tag', [1])[0])
                self.request.url = url_add_query_param(self.request.url, 'test-tag', test_tag_value | (1 << 49), overwrite=True)
                self.add_adb_bits()

        if UrlClass.RTB_AUCTION in self.request.tags:
            if UrlClass.BK_META_AUCTION in self.request.tags and self.meta_callback != 'json':
                self.request.url = url_add_query_param(self.request.url, 'callback', 'json', True)

    @decorators.fixup_url
    @decorators.chkaccess
    @decorators.fixup_scheme
    @decorators.remove_cookies
    @decorators.redirect
    @decorators.accel_redirect
    def proxy_url(self, seed=None, **kwargs):  # noqa
        self.logger.debug("Decrypted url info", crypted_url=self.original_url.geturl(), decrypted_url=self.request.url.geturl())

        if self.request.method == 'POST' and not any([tag in self.request.tags for tag in TAGS_ALLOWED_POST_REQUESTS]):
            self.set_status(405)
            self.finish()
            return

        self.pipeline.put(EventType.APPEND_REQUEST, int(self.start_time_for_pipeline))
        seed = seed or generate_seed(change_period=self.config.SEED_CHANGE_PERIOD,
                                     time_shift_minutes=self.config.SEED_CHANGE_TIME_SHIFT_MINUTES,
                                     salt=self.config.version)
        seed = self.request.headers.get(system_config.SEED_HEADER_NAME, seed)[:SEED_LENGTH]
        ua_data = self.request.user_agent_data
        self.request.bypass = ua_data.isRobot or (self.request.bypass and not self.is_crypted_url)  # в запросах по зашифрованным урлам шифрование не отключаем
        self.metrics.increase_counter('detect_crawler', is_robot=str(ua_data.isRobot), handler='HttpHandler')
        self.metrics.increase_counter('UA', name=ua_data.BrowserName, is_mobile=str(ua_data.isMobile), is_robot=str(ua_data.isRobot), handler='HttpHandler')
        if UrlClass.RTB_AUCTION in self.request.tags or UrlClass.RTB_COUNT in self.request.tags or \
                UrlClass.BK_META_BULK_AUCTION in self.request.tags:
            self.check_ip_headers()
            self.check_uid_cookies()
        if UrlClass.NANPU_COUNT in self.request.tags or UrlClass.NANPU_BK_AUCTION in self.request.tags:
            self.add_adb_bits()
        if UrlClass.RTB_AUCTION in self.request.tags or UrlClass.RTB_COUNT in self.request.tags or \
                UrlClass.BK_VMAP_AUCTION in self.request.tags or UrlClass.RAMBLER_AUCTION in self.request.tags:
            self.proxy_ads_url(seed)
        elif UrlClass.BK_META_BULK_AUCTION in self.request.tags:
            self.proxy_meta_bulk_request(seed)
        elif UrlClass.PCODE_LOADER in self.request.tags:
            self.proxy_pcode_loader_url(seed)
        elif UrlClass.XHR_PROTECTION in self.request.tags:
            self.serve_xhr_protection_fixing_style(seed)
        elif UrlClass.SCRIPT_PROTECTION in self.request.tags:
            self.serve_script_protection_fixing_style(seed)
        elif (not self.request.loggable and self.config.use_cache and not self.request.bypass and
              not self.config.no_cache_url_re_match.match(self.request.url.geturl()) and
              os.path.splitext(self.request.url.path)[1] in system_config.CACHE_EXTENTIONS):
            self.fetch_url_from_cache(seed)
        else:
            self.fetch_url(seed)

    @coroutine
    def fetch_url_from_cache(self, seed):
        start_time = monotonic()
        url = self.request.url.geturl()
        forwarded_proto = self.request.headers.get('X-Forwarded-Proto', '')
        cache_key = ' '.join([forwarded_proto, self.crypted_host, seed, url])
        cache_get_status = None
        crypted_body = None
        headers = None
        try:
            client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
            client_request = tornado.httpclient.HTTPRequest(
                url='http://[::1]:{port}/static_cache?key={key}'.format(key=urlsafe_b64encode(cache_key), port=service_config.SERVICE_PORT),
                method='GET',
                connect_timeout=0.01,
                request_timeout=service_config.REDIS_REQUEST_TIMEOUT_TD.seconds)
            cache_value = yield client.fetch(client_request, raise_error=False)
            if cache_value is not None and cache_value.code == 200:
                cache_get_status = 'hit'
                crypted_body, headers = pickle.loads(cache_value.body)
            else:
                cache_get_status = 'miss'
        except TimeoutError:
            cache_get_status = 'timeout'
        except Exception:
            cache_get_status = 'error'
            self.logger.error('Exception while getting cache', action=action.CACHE_GET, url=url, seed=seed, exc_info=True)
        finally:
            duration = int((monotonic() - start_time) * 1e6)
            self.logger.info(None, action=action.CACHE_GET, url=url, seed=seed, status=cache_get_status, duration=duration)
            self.metrics.increase(action.CACHE_GET, duration, status=cache_get_status)

        if crypted_body is not None and headers is not None:
            self._headers = headers
            self.write(crypted_body)
            self.finish()
            self.logger.info(None, action=self.full_action, http_host=self.request.url.hostname, duration=duration)
            host_for_metrics = format_http_host(self.request.url.hostname) if not self.config.INTERNAL and UrlClass.PARTNER in self.request.tags else self.request.url.hostname
            content_type, _ = cgi.parse_header(headers.get('Content-Type', ''))
            tags_string = ','.join(sorted([tag.name for tag in self.request.tags]))
            self.metrics.increase_counter(self.full_action, http_host=host_for_metrics)
            self.metrics.increase_histogram_by_dimensions(self.full_action, duration, http_host=host_for_metrics, tag=tags_string, content_type=content_type)
        elif cache_get_status == 'miss':
            def _on_set_metric_cb(start_time, *args):
                status = args[0]
                duration = int((monotonic() - start_time) * 1e6)
                self.metrics.increase(action.CACHE_SET, duration=duration, status=str(status))
                self.logger.info(None, duration=duration, action=action.CACHE_SET, status=status, url=url, seed=seed)

            def cache_set_cb(crypted_body, headers, seconds):
                callback = partial(_on_set_metric_cb, monotonic())
                try:
                    cache_value = pickle.dumps((crypted_body, headers))
                    client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
                    client_request = tornado.httpclient.HTTPRequest(
                        url='http://[::1]:{port}/static_cache?key={key}&seconds={seconds}'
                            .format(key=urlsafe_b64encode(cache_key),
                                    seconds=seconds,
                                    port=service_config.SERVICE_PORT),
                        body=cache_value,
                        method='POST',
                        connect_timeout=0.01,
                        request_timeout=service_config.REDIS_REQUEST_TIMEOUT_TD.seconds)
                    client.fetch(client_request, raise_error=False, callback=lambda resp: callback(resp.code))
                    self.logger.debug('Caching response', action=action.CACHE_SET, url=url, seed=seed)
                except Exception:
                    callback('FAILED')
                    self.logger.error('Failed to cache response', action=action.CACHE_SET, exc_info=True)

            self.fetch_url(seed, cache_callback=cache_set_cb)
        else:
            self.fetch_url(seed)

    def serve_xhr_protection_fixing_style(self, seed):
        key = get_key(self.config.CRYPT_SECRET_KEY, seed)
        result = body_replace(v_protect.FIX_CSS, key, replace_dict={v_protect.crypt_body_re: None}, charset='ascii', crypt_in_lowercase=self.config.CRYPT_IN_LOWERCASE)
        self.write(result)
        self.finish()

    def serve_script_protection_fixing_style(self, seed):
        key = get_key(self.config.CRYPT_SECRET_KEY, seed)
        scheme, _ = cgi.parse_header(self.request.headers.get('X-Forwarded-Proto', 'http'))
        # урл может быть с квериаргом, в котором есть nonce для style
        nonce = parse_qs(self.request.url.query).get(v_protect.NONCE_QUERY_ARG_NAME, [''])[0]
        nonce_css = "a.setAttribute(\"nonce\", \"{}\");".format(nonce) if nonce else ''
        binurlprefix = CryptUrlPrefix(scheme, self.crypted_host, seed, prefix=self.prefix)
        if self.config.xhr_protection_on:
            body = v_protect.FIX_JS_WITH_XHR.format(SRC=v_protect.XHR_FIX_STYLE_SRC, NONCE_2=nonce_css)
        else:
            body = v_protect.FIX_JS.format(NONCE_2=nonce_css)

        result = body_crypt(
            body=body,
            binurlprefix=binurlprefix,
            key=key,
            crypt_url_re=v_protect.crypt_url_re,
            enable_trailing_slash=self.config.CRYPT_ENABLE_TRAILING_SLASH,
            min_length=self.config.RAW_URL_MIN_LENGTH,
            crypted_url_mixing_template=self.config.CRYPTED_URL_MIXING_TEMPLATE,
            config=self.config,
            partner_cookieless_url_re_match=self.config.partner_to_cookieless_host_urls_re_match,
        )
        result = body_replace(result, key, replace_dict={v_protect.crypt_body_re: None}, charset='ascii', crypt_in_lowercase=self.config.CRYPT_IN_LOWERCASE)

        if self.config.encryption_step_enabled(system_config.EncryptionSteps.loaded_js_crypt):
            result = crypt_js_body(result, seed, charset='ascii', remove_after_run=self.config.REMOVE_SCRIPTS_AFTER_RUN)
        self._headers['Content-Type'] = 'application/javascript'
        self.write(result)
        self.finish()

    def proxy_meta_bulk_request(self, seed):
        meta_bulk_req_body = json.loads(self.request.body)
        adb_bits_value = int(meta_bulk_req_body["requestData"]["common"]["queryParameters"].get("adb-bits", "1")) | 1
        bk_disadbled_ad_types = int(meta_bulk_req_body["requestData"]["common"]["queryParameters"].get(bk_config.BK_DISABLE_AD_PARAM, "0")) | self.config.DISABLED_BK_AD_TYPES
        query_parameters = dict(
            (
                bk_config.ADB_ENABLED_FLAG.split('='),
                bk_config.BK_URL_PARAMS.split('='),
                ("adb-bits", adb_bits_value),
                (bk_config.BK_DISABLE_AD_PARAM, bk_disadbled_ad_types),
            )
        )
        meta_bulk_req_body["requestData"]["common"]["queryParameters"].update(query_parameters)
        self.request.body = json.dumps(meta_bulk_req_body)
        self.fetch_url(seed)

    def is_yandexuid_present(self):
        return self.request.cookies.get(system_config.YAUID_COOKIE_NAME, None) or self.request.headers['Cookie'].find(system_config.YAUID_COOKIE_NAME) > 0

    def proxy_pcode_loader_url(self, seed):
        if not self.is_yandexuid_present():
            crookie = self.get_crypted_yauid_cookie_value()
            if crookie is not None:
                decrypted_cookie = self.cookie_encrypter.decrypt_cookie(crookie)
                if decrypted_cookie:
                    # Куки приходится доклеивать через хедер, а не через request.cookies, так как request.cookies - это специальный объект для чтения кук, а не просто словарик T_T
                    self.request.headers['Cookie'] += '; {}={}'.format(system_config.YAUID_COOKIE_NAME, decrypted_cookie)
                    self.metrics.increase_counter('decrypting_yauid', success=1)
                else:
                    self.logger.warning('Failed to decrypt cookie: {}'.format(crookie))
                    self.metrics.increase_counter('decrypting_yauid', success=0)
        self.fetch_url(seed)

    def proxy_ads_url(self, seed):
        # https://st.yandex-team.ru/ANTIADB-1940
        if not self.config.DISABLE_ADB_ENABLED or UrlClass.YABS_COUNT not in self.request.tags:
            self.request.url = url_add_param(self.request.url, bk_config.ADB_ENABLED_FLAG)  # https://st.yandex-team.ru/ANTIADB-500
        # сохраним квери параметр callback, чтобы затем в него завернуть JSONP
        if UrlClass.RTB_AUCTION in self.request.tags or UrlClass.RAMBLER_AUCTION in self.request.tags:
            args = parse_qs(self.request.url.query)
            callbacks = args.get('callback', [None])
            self.meta_callback = callbacks[0]
            if self.rtb_auction_via_script and self.meta_callback is None:
                # если ожидается запрос за рекламой скриптом, но он прилетает без квери-арга callback, то считаем его XHR-запросом
                self.rtb_auction_via_script = False
                self.logger.error("Callback qarg is not found in RTB link.", action=action.RTB_AUCTION_VIA_SCRIPT, url=self.request.url.geturl())
                self.metrics.increase_counter(action.RTB_AUCTION_VIA_SCRIPT, success=0)
            # https://st.yandex-team.ru/ANTIADB-2418
            if UrlClass.BK in self.request.tags and UrlClass.ADFOX not in self.request.tags and bk_config.JSONP_PARAM_VALUE in args.get(bk_config.JSONP_PARAM, []):
                self.rtb_jsonp_response = True
        # Check if yandexuid is already in request
        if self.is_yandexuid_present():
            self.logger.add_to_context(cm_type="with_yauid")
            self.metrics.increase_counter(action.COOKIE_MATCHING, type='with_yauid')
            self.yandex_ads_fetch_yandexuid(seed)

        # Check if crypted yandexuid is in request
        elif self.get_crypted_yauid_cookie_value():
            self.logger.add_to_context(cm_type="with_crypted_yauid")
            self.metrics.increase_counter(action.COOKIE_MATCHING, type='with_crypted_yauid')
            self.yandex_ads_fetch_crypted_yandexuid(seed)

        elif self.get_extuid_cookie_value():
            self.logger.add_to_context(cm_type="with_extuid")
            self.metrics.increase_counter(action.COOKIE_MATCHING, type='with_extuid')
            self.yandex_ads_fetch_extuid(seed, self.get_extuid_cookie_value())

        else:
            self.logger.add_to_context(cm_type="without_uid")
            self.metrics.increase_counter(action.COOKIE_MATCHING, type='without_uid')
            # fetch ads with no cookie
            self.yandex_ads_fetch_extuid(seed, extuid_value=None)


# noinspection PyAbstractClass
class StaticMonHandler(HttpHandler):
    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(StaticMonHandler, self).initialize(config, **kwargs)
        self.full_action = action.HANDLER_DETECT_SCRIPT
        # https://st.yandex-team.ru/ANTIADB-1959
        if config.AUTO_SELECT_DETECT:
            new_path = system_config.WITHOUT_CM_DETECT_LIB_PATH if config.CM_DISABLED else system_config.WITH_CM_DETECT_LIB_PATH
            self.original_url = self.original_url._replace(path=new_path)
            self.request.url = self.original_url
        # https://st.yandex-team.ru/ANTIADB-1829
        if config.new_detect_script_url is not None and service_config.DC in config.new_detect_script_dc:
            self.original_url = config.new_detect_script_url
            self.request.url = self.original_url

    @tornado.web.asynchronous
    def get(self):
        self.proxy_url()

    # обработка body полученного на request от запроса прокси в методе fetch_url
    def process_response(self, seed, response, http_client, *args):
        http_client.close()
        config = self.config
        request = self.request
        self.last_response = response
        try:
            self._headers = response.headers
            remove_headers(self._headers, config.deny_headers_backward)
            remove_headers(self._headers, config.deny_headers_backward_proxy)
            self.content_type, content_params = cgi.parse_header(response.headers.get('Content-Type', ''))
            # Last-Modified on staticmon script is staticmon realease datetime
            self._headers['Last-Modified'] = strftime("%a, %d %b %Y %X GMT", gmtime())  # like 'Fri, 06 Apr 2018 13:52:20 GMT'
            if response.code != 200:
                # trying read detect script from file cache
                with open(os.path.join(service_config.PERSISTENT_VOLUME_PATH, url_to_filename(request.url.path)), mode='rb') as f:
                    result = f.read()
                self.logger.info("Cryprox using script detect from cache",
                                 action=action.FETCH_CONTENT,
                                 http_host=request.url.hostname,
                                 http_code=response.code,
                                 human_readable_reason=response.reason,
                                 exception=str(response.error),
                                 url=response.effective_url)
                self.content_type = 'text/javascript'
            else:
                result = response.body
            self.set_status(200, 'OK')
            self.init_binurlprefix_with_crypt_key(self.crypted_host, seed, request.headers)
            crypted_body = self.crypt_response(seed, result, content_params)
            self.write(crypted_body)
            self.finish()
            duration = int((monotonic() - self.start_time) * 10 ** 6)
            self.logger.info(None, action=self.full_action, duration=duration)
            self.metrics.increase(self.full_action, duration)
        except tornado.httpclient.HTTPError:
            raise
        except Exception:
            self.logger.error("Exception in static-mon handle request")
            raise


# noinspection PyAbstractClass
class CryHandler(HttpHandler):

    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(CryHandler, self).initialize(config, **kwargs)
        self.full_action = action.HANDLER_CRY
        self.check_uid()

    def check_uid(self):
        config = self.config
        experiment_on = config.experiment_on and is_experiment_date(config.experiment_range)
        if not config.bypass_by_uids and not experiment_on:
            return

        uid = self.get_uid()
        if uid is None:
            return

        uid_type = '-'
        experiment_type = '-'
        ua_data = self.request.user_agent_data
        if config.bypass_by_uids and BypassByUids().has(ua_data.device, uid, metrics=self.metrics):
            uid_type = 'bypass_by_uid'
            self.request.bypass = True
            if experiment_on and config.experiment_type == system_config.Experiments.NOT_BYPASS_BYPASS_UIDS and ua_data.device in config.experiment_device:
                # Контрольная и экспериментальная группы уидов будут отфильтрованы из логов прокси по условию:
                # action='uid_check' and experiment_type='NOT_BYPASS_BYPASS_UIDS' and (uid_type='control' or uid_type='experimental')
                experiment_type = system_config.Experiments.NOT_BYPASS_BYPASS_UIDS.name
                seed = config.experiment_range[0].toordinal()
                if uid_is_experimental(uid, seed=seed, percent=config.experiment_percent, metrics=self.metrics):
                    uid_type = 'experimental'
                    self.request.bypass = False
                elif uid_is_control(uid, seed=seed, percent=config.experiment_percent):
                    uid_type = 'control'
        elif experiment_on:
            if config.experiment_type == system_config.Experiments.BYPASS and ua_data.device in config.experiment_device:
                experiment_type = system_config.Experiments.BYPASS.name
                seed = config.experiment_range[0].toordinal()
                if uid_is_experimental(uid, seed=seed, percent=config.experiment_percent, metrics=self.metrics):
                    uid_type = 'experimental'
                    self.request.bypass = True
                elif uid_is_control(uid, seed=seed, percent=config.experiment_percent):
                    uid_type = 'control'
            elif config.experiment_type == system_config.Experiments.FORCECRY:
                experiment_type = system_config.Experiments.FORCECRY.name
                cookie_forcecry = self.request.cookies.get('forcecry', None)
                if cookie_forcecry is not None and cookie_forcecry.value == '1':
                    uid_type = 'experimental'
        self.logger.info(None, action='uid_check', uid=uid, uid_type=uid_type, experiment_type=experiment_type, is_mobile=ua_data.isMobile, ua=ua_data.BrowserName)

    def do_proxy_request(self):
        exception = None
        seed = None
        try:
            decrypted_url, seed, origin = self.decrypt_url()

            # Если сrypted_host не задан явно и мы получили origin из ссылки, будем использовать его для шифрования
            if self.request.headers.get(system_config.CRYPTED_HOST_HEADER_NAME) is None and self.config.CRYPTED_HOST is None:
                self.crypted_host = origin or self.crypted_host

            if decrypted_url is None:
                self.logger.debug("Failed to decrypt url. Proxing 'as is'")
                self.is_crypted_url = False
                self.proxy_url()
                return

            self.is_crypted_url = True
            decrypted_url = self.append_hidden_args(decrypted_url)
            self.request.url = urlparse(decrypted_url)
            self.request.query = None
        except DecryptionError as ex:
            # we shouldn't get here if url is not crypted
            exception = ex
            self.logger.exception("Failed to xor decrypt url.", url=self.request.uri, cause=ex.cause)
        finally:
            debug_token = self.request.headers.get(system_config.DEBUG_TOKEN_HEADER_NAME)
            if debug_token is not None:
                # Dry run mode. Do not proxy this request set debug header instead
                self.finish_with_debug_info(debug_token, exception)
                return
        self.proxy_url(seed=seed)

    @tornado.web.asynchronous
    def get(self):
        self.do_proxy_request()

    @tornado.web.asynchronous
    def post(self):
        self.do_proxy_request()

    def finish_with_debug_info(self, debug_token, exception):
        if debug_token != system_config.DEBUG_API_TOKEN:
            self.set_status(403)
            self.finish('Forbidden')
        else:
            if exception is not None:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                result = "".join(traceback.format_exception(exc_type, exc_value, exc_traceback, limit=2))
            else:
                result = self.request.url.geturl()
            self.set_header(system_config.DEBUG_RESPONSE_HEADER_NAME, result)
            self.finish()
        return

    @logged_action(action_name=action.DECRYPTURL)
    def decrypt_url(self):
        config = self.config
        # расшифровывать будем урл без протокола и домена
        # TODO: use coroutine context logger instead of instance
        # TODO: передавать хедеры в либу расшивровки, чтобы она доклеивала скрытую часть урла https://st.yandex-team.ru/ANTIADB-1782

        return decrypt_url(urlparse(self.request.uri)._replace(scheme='', netloc='').geturl(),
                                          str(config.CRYPT_SECRET_KEY), str(config.CRYPT_PREFFIXES), config.CRYPT_ENABLE_TRAILING_SLASH)

    def finish(self, chunk=None):
        self._headers[system_config.NGINX_SERVICE_ID_HEADER] = self.config.name  # https://st.yandex-team.ru/ANTIADB-2013
        result = super(CryHandler, self).finish(chunk)
        if self.last_response and os.urandom(2) == '\0\0':  # sample down to to 1/65536
            detected_charset = chardet.detect(self.last_response.body)['encoding']
            _, content_params = cgi.parse_header(self.last_response.headers.get('Content-Type', ''))
            self.metrics.increase_counter(action=self.full_action,
                                          detected_charset=detected_charset,
                                          header_charset=content_params.get('charset', None))
        return result


# noinspection PyAbstractClass
class ArgusContentHandler(CryHandler):
    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(ArgusContentHandler, self).initialize(config, **kwargs)
        self.is_argus_handler = True
        self.params_json_settings = {
            "layout-config": ["ad_no", "req_no"]
        }

    def _get_cache_expire(self, cache_control):
        return 30*60

    def filter_url_for_cache(self, url_f):
        filter_params = url_keep_query_params(urlparse(url_f), self.config.url_keep_query_params)
        return url_keep_json_keys_in_query_params(filter_params, self.params_json_settings).geturl()

    def get_cache_key(self, url, execution_id):
        if self.config.rtb_auction_url_re_match.match(url):
            return self.filter_url_for_cache(url) + ' ' + execution_id
        return url + ' ' + execution_id

    def process_response(self, seed, response, http_client, cache_callback=None):
        if UrlClass.RTB_COUNT in self.request.tags:
            adbbits = int(parse_qs(self.request.url.query).get('adb-bits', [1])[0])
            url = bk_decode(self.request.url.geturl())
            flags = url.rstrip('/').split(',')[1:-1]
            flags = dict(x.split('=', 1) for x in flags)
            self.logger.info("ELASTIC: RTB_COUNT", flags=flags,
                             adbbits=adbbits >> 1,
                             action=action.ARGUS_GREP_LOGS)
        if UrlClass.RTB_AUCTION in self.request.tags:
            adbbits = int(parse_qs(self.request.url.query).get('adb-bits', [1])[0])
            response_json = json.loads(response.body)
            self.logger.info("ELASTIC: RTB_AUCTION", url=self.request.url.geturl(),
                             rtbAuctionInfo=response_json.get("rtbAuctionInfo", {}),
                             adbbits=adbbits >> 1,
                             action=action.ARGUS_GREP_LOGS)
        super(ArgusContentHandler, self).process_response(seed, response, http_client, cache_callback)

    def fetch_url_and_save_response(self, execution_id, seed):
        url = self.request.url.geturl()
        self.logger.debug('ARGUS: Cookie standard exists', execution_id=execution_id)
        cache_key = self.get_cache_key(url, execution_id)
        self.logger.debug('ARGUS: Generate cache_key', cache_key=cache_key)

        def cache_set_cb(crypted_body, headers, seconds):
            try:
                cache_value = pickle.dumps((crypted_body, headers))
                client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
                client_request = tornado.httpclient.HTTPRequest(
                    url='http://[::1]:{port}/argus_cache?key={key}&seconds={seconds}'
                        .format(key=urlsafe_b64encode(cache_key),
                                seconds=30 * 60,
                                port=service_config.SERVICE_PORT),
                    body=cache_value,
                    method='POST',
                    connect_timeout=service_config.REDIS_CONNECT_TIMEOUT,
                    request_timeout=service_config.REDIS_REQUEST_TIMEOUT_TD.seconds)
                http_retry(client, client_request, raise_error=False, tries=3, delay=1, backoff=3)
                self.logger.debug('ARGUS: Caching response', action=action.ARGUS_SET, url=url, seed=seed)
            except Exception:
                self.logger.error('ARGUS: Failed to cache response', action=action.ARGUS_SET, exc_info=True)

        super(ArgusContentHandler, self).fetch_url(seed, cache_callback=cache_set_cb)

    @coroutine
    def fetch_url_from_argus_cache(self, execution_id, seed):
        url = self.request.url.geturl()
        self.logger.debug('ARGUS: Cookie replay exists', execution_id=execution_id)
        cache_key = self.get_cache_key(url, execution_id)
        self.logger.debug('ARGUS: Generate cache_key', cache_key=cache_key)
        crypted_body = None
        headers = None
        try:
            client = tornado.httpclient.AsyncHTTPClient(force_instance=True)
            request = tornado.httpclient.HTTPRequest(
                url='http://[::1]:{port}/argus_cache?key={key}'.format(key=urlsafe_b64encode(cache_key),
                                                                       port=service_config.SERVICE_PORT),
                method='GET',
                connect_timeout=service_config.REDIS_CONNECT_TIMEOUT,
                request_timeout=service_config.REDIS_REQUEST_TIMEOUT_TD.seconds)
            response = yield http_retry(client, request, raise_error=False, tries=3, delay=1, backoff=3)
            self.logger.debug('ARGUS: Get from redis', status_code=response.code)
            if response is not None and response.code == 200:
                crypted_body, headers = pickle.loads(response.body)
                self.logger.debug('ARGUS: Cache ok', action=action.ARGUS_GET, url=url, seed=seed)
        except Exception as ex:
            self.logger.debug('ARGUS: Exception while getting cache', exc=ex.message,
                              action=action.ARGUS_GET, url=url, seed=seed, exc_info=True)

        if crypted_body is not None and headers is not None:
            self._headers = headers
            self.write(crypted_body)
            self.finish()
        else:
            # not found in redis
            self.logger.debug('ARGUS: go to super fetch-url')
            super(ArgusContentHandler, self).fetch_url(seed)

    def get_execution_id(self, cookie_name, header_name):
        cookie = self.request.cookies.get(cookie_name)
        return cookie.value if cookie else self.request.headers.get(header_name)

    def fetch_url(self, seed, cache_callback=None):
        if UrlClass.RTB_COUNT in self.request.tags or UrlClass.COUNTERS in self.request.tags:
            super(ArgusContentHandler, self).fetch_url(seed, cache_callback)
            return

        execution_id = self.get_execution_id('argus_standard', system_config.ARGUS_STANDARD_HEADER_NAME)
        if execution_id is not None:
            # reference version
            self.fetch_url_and_save_response(execution_id, seed)
            return

        execution_id = self.get_execution_id('argus_replay', system_config.ARGUS_REPLAY_HEADER_NAME)
        if execution_id is not None:
            # repeat version
            self.fetch_url_from_argus_cache(execution_id, seed)
            return

        self.logger.debug('ARGUS: NO COOKIE')

    def finish(self, *args, **kwargs):
        self._headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
        return super(ArgusContentHandler, self).finish(*args, **kwargs)


# noinspection PyAbstractClass
class CryptContentHandler(CryHandler):

    def initialize(self, config, **kwargs):
        super(CryptContentHandler, self).initialize(config, **kwargs)
        self.full_action = action.HANDLER_CRYPT_CONTENT

    @tornado.web.asynchronous
    def post(self):
        url = self.request.headers.get(system_config.CRYPTED_URL_HEADER_NAME)

        if url is not None:
            (self.request.tags, duration) = duration_usec(classify_url, self.config, url)
            self.logger.request_tags += [tag.name for tag in self.request.tags]
        else:
            url = self.request.url.geturl()

        seed = generate_seed(change_period=self.config.SEED_CHANGE_PERIOD,
                             time_shift_minutes=self.config.SEED_CHANGE_TIME_SHIFT_MINUTES,
                             salt=self.config.version)
        seed = self.request.headers.get(system_config.SEED_HEADER_NAME, seed)[:SEED_LENGTH]
        self.content_type, content_params = cgi.parse_header(self.request.headers.get('Content-Type', ''))
        self.init_binurlprefix_with_crypt_key(self.crypted_host, seed, self.request.headers)
        crypted_body = self.crypt_body(seed, self.request.body, content_params, url, self.request.headers)
        for k, v in self.request.headers.items():
            if k.lower() in system_config.CSP_HEADER_KEYS:
                self.set_header(k, v)
        self.write(crypted_body)
        self.finish()
        duration = int((monotonic() - self.start_time) * 10 ** 6)
        self.logger.info(None, action=self.full_action, url=url, duration=duration)
        self.metrics.increase(self.full_action, duration)


# noinspection PyAbstractClass
class BaseHandler(tornado.web.RequestHandler):

    # noinspection PyMethodOverriding
    # noinspection PyAttributeOutsideInit
    def initialize(self, service_id, request_tags):
        # https://st.yandex-team.ru/ANTIADB-2402
        self.end2end_logger = None
        if service_config.END2END_TESTING:
            request_id = self.request.headers.get(system_config.REQUEST_ID_HEADER_NAME, 'None')
            self.end2end_logger = JSLogger('End2EndLogger', log_file=service_config.END2END_TESTING_LOG_FILE, request_id=request_id,
                                           service_id=service_id, request_tags=request_tags)

    def finish(self, chunk=None):
        if self.end2end_logger is not None:
            self.end2end_logger.info('', body=''.join(self._write_buffer), status_code=self.get_status())
        super(BaseHandler, self).finish(chunk)


# noinspection PyAbstractClass
class CookieHandler(BaseHandler):

    # noinspection PyMethodOverriding
    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(CookieHandler, self).initialize(config.name, ['COOKIE_OF_THE_DAY'])
        self.config = config

    @tornado.web.asynchronous
    def get(self):
        self.set_status(200)
        self.write(self.config.CURRENT_COOKIE)
        self._headers[system_config.NGINX_SERVICE_ID_HEADER] = self.config.name  # https://st.yandex-team.ru/ANTIADB-2013
        self.finish()


# noinspection PyAbstractClass
class CryptUrlHandler(HttpHandler):

    # noinspection PyMethodOverriding
    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(CryptUrlHandler, self).initialize(config, **kwargs)
        request_qargs = {key: value for key, value in parse_qsl(self.original_url.query)}
        self.url_for_crypt = request_qargs.get("url", "")
        self.crypted_host = request_qargs.get("host", "")

    @tornado.web.asynchronous
    def get(self):
        if not self.url_for_crypt or not self.crypted_host:
            self.set_status(400)
            self.write("Request validation fail due to miss param: url or host")
            self.finish()
            return

        url = urlparse(self.url_for_crypt)
        proto = url.scheme if url.scheme in ['http', 'https'] else 'https'
        domain = url.netloc
        path = url.path
        query = url.query

        if not domain or not path:
            self.set_status(400)
            self.write("Request validation fail due to invalid param: domain or path not exists in url - {}".format(self.url_for_crypt))
            self.finish()
            return

        path += "?" + query.replace('&', '%26') if query else ""
        config = self.config

        seed = generate_seed(change_period=config.SEED_CHANGE_PERIOD,
                             time_shift_minutes=config.SEED_CHANGE_TIME_SHIFT_MINUTES,
                             salt=self.config.version)
        seed = self.request.headers.get(system_config.SEED_HEADER_NAME, seed)[:SEED_LENGTH]
        key = get_key(config.CRYPT_SECRET_KEY, seed)
        host = self.crypted_host.replace('.', '-').replace('_', '-') + '.' + config.PARTNER_COOKIELESS_DOMAIN
        binurlprefix = CryptUrlPrefix(scheme=proto, host=host, seed=seed, prefix='/', second_domain=self.crypted_host)
        url_for_crypt = "{scheme}://{host}/detect?domain={domain}&path={path}&proto={scheme}".format(scheme=proto, host=self.crypted_host, domain=domain, path=path)
        crypted_url = crypt_url(binurlprefix, url_for_crypt, key, False, config.RAW_URL_MIN_LENGTH, config.CRYPTED_URL_MIXING_TEMPLATE)
        self.logger.debug(None,
                          action=action.HANDLER_CRYPT_URL,
                          original_url=self.original_url.geturl(),
                          url_for_crypt=url_for_crypt,
                          crypted_url=crypted_url,
                          )
        duration = int((monotonic() - self.start_time) * 10 ** 6)
        self.metrics.increase(action.HANDLER_CRYPT_URL, duration=duration)
        self.set_status(200)
        self.write(crypted_url)

        self.finish()


# noinspection PyAbstractClass
class GenerateCookieHandler(HttpHandler):
    # noinspection PyAttributeOutsideInit
    def initialize(self, config, **kwargs):
        super(GenerateCookieHandler, self).initialize(config, **kwargs)

    @tornado.web.asynchronous
    def get(self):
        key = self.config.CRYPT_SECRET_KEY
        request_qargs = parse_qs(self.request.url.query)
        script_key = request_qargs.get("script_key", [""])[0]
        key_is_valid, message = validate_script_key(script_key, key)
        if key_is_valid:
            self.metrics.increase_counter(action.VALIDATE_SCRIPT_KEY, success=1)
            if self.request.headers.get("X-Real-Ip", ""):
                client_ip = self.request.headers.get("X-Real-Ip")
            elif self.request.headers.get("X-Forwarded-For-Y", ""):
                client_ip = self.request.headers.get("X-Forwarded-For-Y")
            else:
                client_ip = self.request.headers.get("X-Forwarded-For", "").split(",")[0]
            # ограничим длину данных из заголовков
            client_ip = client_ip[:50]
            user_agent = self.request.headers.get("User-Agent", "")[:300]
            accept_language = self.request.headers.get("Accept-Language", "")[:100]
            generate_time = int(time())
            cookie_value = get_crypted_cookie_value(self.config.COOKIE_CRYPT_KEY, client_ip, user_agent, accept_language, generate_time)
            duration = int((monotonic() - self.start_time) * 10 ** 6)
            self.logger.debug(None,
                              action=action.GET_CRYPTED_COOKIE,
                              duration=duration,
                              crypted_cookie=cookie_value,
                              client_ip=client_ip,
                              user_agent=user_agent,
                              accept_language=accept_language,
                              generate_time=generate_time,
                              )
            ua_data = parse_user_agent_data(user_agent)
            self.metrics.increase_histogram(action.GET_CRYPTED_COOKIE, duration=duration)
            self.metrics.increase_counter('detect_crawler', is_robot=str(ua_data.isRobot), handler='GenerateCookieHandler')
            self.metrics.increase_counter('UA', name=ua_data.BrowserName, is_mobile=str(ua_data.isMobile), is_robot=str(ua_data.isRobot), handler='GenerateCookieHandler')
            self.metrics.increase(action.GET_CRYPTED_COOKIE, duration=duration, cookie_is_new=0 if request_qargs.get("reasure", [""])[0] == "true" else 1, crawler=str(ua_data.isRobot))
            self.set_status(200)
            self.write(cookie_value)
        else:
            self.logger.debug("Validate script key is failed, reason: {}".format(message),
                              action=action.VALIDATE_SCRIPT_KEY,
                              script_key=script_key,
                              )
            self.metrics.increase_counter(action.VALIDATE_SCRIPT_KEY, success=0)
            self.set_status(403)
        self.finish()


# noinspection PyAbstractClass
class CookieCryptHandler(BaseHandler):
    # 422 code for backward compatibility with cookie matcher service
    FAILURE_CODE = 422

    # noinspection PyMethodOverriding
    # noinspection PyAttributeOutsideInit
    def initialize(self):
        super(CookieCryptHandler, self).initialize(None, ['ENCRYPT_COOKIE'])
        self.cookie_encrypter = CookieEncrypter(service_config.COOKIE_ENCRYPTER_KEYS_PATH)
        self.logger = JSLogger('root')
        self.logger.setLevel(service_config.LOGGING_LEVEL)
        self.start_time = monotonic()

    @tornado.web.asynchronous
    def get(self):
        yandexuid_cookie = self.request.cookies.get(system_config.YAUID_COOKIE_NAME, None)
        if yandexuid_cookie:
            encrypted_yuid = self.cookie_encrypter.encrypt_cookie(yandexuid_cookie.value)
            if encrypted_yuid:
                self.write(encrypted_yuid)
                self.set_status(200)
            else:
                self.set_status(status_code=CookieCryptHandler.FAILURE_CODE, reason='failed')
                self.logger.warning('Failed to encrypt cookie: {}'.format(yandexuid_cookie.value))
        else:
            self.set_status(status_code=CookieCryptHandler.FAILURE_CODE, reason='empty')
            self.logger.warning('No cookie passed')
        self.finish()


# noinspection PyAbstractClass
class WorkerControlHandler(tornado.web.RequestHandler):
    SUPPORTED_METHODS = ['POST']

    # noinspection PyAttributeOutsideInit
    def initialize(self):
        self.request_id = self.request.headers.get(system_config.REQUEST_ID_HEADER_NAME, 'None')
        self.request.tags = []
        self.logger = JSLogger('root')
        self.logger.setLevel(service_config.LOGGING_LEVEL)
        self.start_time = monotonic()

    @coroutine
    def post(self, *args, **kwargs):
        if "reload_configs" in self.request.uri:
            yield self.application.update_configs()
            self.set_status(200)
            self.write('''{"status": "reloaded"}''')
            self.finish()
        elif "update_bypass_uids" in self.request.uri:
            yield BypassByUids().update_uids()
            self.set_status(200)
            self.write('''{"status": "bypass_uids_updated"}''')
            self.finish()
        elif "update_experiment_config" in self.request.uri:
            yield InternalExperiment().update_config()
            self.set_status(200)
            self.write('''{"status": "experiment_config_updated"}''')
            self.finish()
        else:
            self.set_status(405)
            self.finish()


class WorkerApplication(tornado.web.Application):

    @coroutine
    def update_configs(self, configs=None):
        from antiadblock.cryprox.cryprox.common.config_utils import get_configs

        if configs is None:
            configs = yield get_configs(service_config.CONFIGSAPI_CACHE_URL)

        # N.B. Looks fooky, this is not atomic
        self.configs_cache = configs
        token_map = defaultdict(dict)
        naydex_subdomain_map = defaultdict(dict)
        for key, config in configs.iteritems():
            for token in config.PARTNER_TOKENS:
                token_map[token][key] = config
                naydex_subdomain_map[config.name.replace('.', '-').replace('_', '-')][key] = config
        self.token_crypt_content_router.token_map = token_map
        self.token_router.token_map = token_map
        self.cookie_router.token_map = token_map
        self.crypt_url_router.token_map = token_map

        autoredirect_config = configs.get(system_config.AUTOREDIRECT_SERVICE_ID) or configs.get(system_config.AUTOREDIRECT_KEY)
        if autoredirect_config is not None:
            for item in autoredirect_config.webmaster_data.keys():
                naydex_subdomain_map[item.replace('.', '-').replace('_', '-')][system_config.AUTOREDIRECT_KEY] = autoredirect_config
        self.naydex_router.subdomain_map = naydex_subdomain_map

        configure_http_client(configs)
