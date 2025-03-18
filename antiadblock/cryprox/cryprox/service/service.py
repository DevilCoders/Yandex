# -*- coding: utf8 -*-

"""
http server for income requests from partner webserver(nginx|apache)
"""
import logging
from abc import ABCMeta, abstractmethod
from collections import defaultdict

from re2 import escape as re_escape
from re2 import compile as re_compile

from tornado.web import _ApplicationRouter, ErrorHandler
from tornado.routing import Rule, AnyMatches, PathMatches, Matcher

from .worker_application import WorkerApplication, configure_http_client, CryHandler, StaticMonHandler, \
    WorkerControlHandler, CookieHandler, GenerateCookieHandler, CookieCryptHandler, \
    CryptUrlHandler, CryptContentHandler, ArgusContentHandler
from antiadblock.cryprox.cryprox.config.service import ENV_TYPE
from .service_application import PingHandler, ServiceHandler, ConfigsHandler, ServiceApplication, ControlHandler, \
    CacheControlHandler, MetricsHandler, SystemMetricsHandler, StaticCacheHandler, ArgusCacheHandler
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.config_utils import select_configs
from antiadblock.cryprox.cryprox.common.tools.ip_utils import check_ip_headers_are_internal, ip_headers_is_internal
from antiadblock.cryprox.cryprox.common.tools.ua_detector import get_ua_data_and_device

cookieless_regexp = re_compile(system_config.COOKIELESS_HOST_RE)


def make_worker_app(configs, pipeline):

    class UriMatcher(Matcher):

        """Matches requests with uri specified by ``uri_pattern`` regex."""

        def __init__(self, uri_pattern):
            self.regex = re_compile(uri_pattern)

        def match(self, request):
            match = self.regex.match(request.uri)
            if match is None:
                return None
            return {}

    class ForbiddenHandler(ErrorHandler):

        # noinspection PyMethodOverriding
        def initialize(self):
            self.set_status(403)
            # TODO: the attentive reader might notice some code seizing from antiadblock.cryprox.cryprox.service.decorators:chkaccess
            self.reason = 'Forbidden\n'

        def write_error(self, status_code, **kwargs):
            self._headers[system_config.NGINX_SERVICE_ID_HEADER] = 'unknown'
            self.finish(self.reason)

    class PidRouter(_ApplicationRouter):
        """
        Abstract class for routing requests like http://some.thing?pid=service_id
        """
        __metaclass__ = ABCMeta

        def find_handler(self, request, **kwargs):
            services_ids = request.query_arguments.get('pid', [])
            if not services_ids:
                logging.warning("got request without a pid")
                return self.application.get_handler_delegate(request, ForbiddenHandler)

            user_agent_data, device_type = get_ua_data_and_device(request.headers.get('User-Agent'))
            service_configs = {key: config for key, config in self.application.configs_cache.iteritems() if config.name in services_ids}
            if not service_configs:
                logging.warning("unknown services ids (pid) received: {}".format(services_ids))
                return self.application.get_handler_delegate(request, ForbiddenHandler)
            config = select_configs(service_configs, device_type)
            if not config:
                logging.warning("Cannot select config for device_type: {}, {}".format(token, device_type))
                return self.application.get_handler_delegate(request, ForbiddenHandler)

            return self.application.get_handler_delegate(request, self.get_handler_class(), target_kwargs=dict(config=config, pipeline=pipeline, ua_data=user_agent_data))

        @abstractmethod
        def get_handler_class(self):
            raise NotImplementedError

    class StaticMonApplicationRouter(PidRouter):
        def get_handler_class(self):
            return StaticMonHandler

    class GenerateCookieApplicationRouter(PidRouter):
        def get_handler_class(self):
            return GenerateCookieHandler

    def is_argus_action(request):
        return request.cookies.get('argus_standard', request.cookies.get('argus_replay', None)) is not None \
            or request.headers.get(system_config.ARGUS_REPLAY_HEADER_NAME) is not None \
            or request.headers.get(system_config.ARGUS_STANDARD_HEADER_NAME) is not None

    class TokenApplicationRouter(_ApplicationRouter):
        def __init__(self, application, delegate_handler, rules=None, token_map=None):
            if token_map is None:
                token_map = {}
            self.token_map = token_map
            self.delegate_handler = delegate_handler
            super(TokenApplicationRouter, self).__init__(application, rules)

        def select_handler(self, request):
            return ArgusContentHandler if is_argus_action(request) else self.delegate_handler

        def find_handler(self, request, **kwargs):
            token = request.headers.get(system_config.PARTNER_TOKEN_HEADER_NAME)
            if not token:
                logging.warning("got request without a token")
                return self.forbidden(request)

            service_configs = self.token_map.get(token)
            if not service_configs:
                logging.warning("unknown token received: {}".format(token))
                return self.forbidden(request)

            # Для одного сервиса может быть несколько конфигов, нужно выбрать в соответствии с device_type
            user_agent_data, device_type = get_ua_data_and_device(request.headers.get('User-Agent'))
            checked_ip_headers = check_ip_headers_are_internal(system_config.YANDEX_NETS, request.headers)
            config = select_configs(service_configs, device_type, exp_id=request.headers.get(system_config.EXPERIMENT_ID_HEADER),
                                    is_internal=ip_headers_is_internal(checked_ip_headers))
            if not config:
                logging.warning("Cannot select config for device_type: {}, {}".format(token, device_type))
                return self.forbidden(request)
            return self.application.get_handler_delegate(request, self.select_handler(request), target_kwargs=dict(
                config=config, pipeline=pipeline, ua_data=user_agent_data, checked_ip_headers=checked_ip_headers))

        def forbidden(self, request):
            return self.application.get_handler_delegate(request, ForbiddenHandler)

    class SubdomainApplicationRouter(_ApplicationRouter):
        def __init__(self, application, delegate_handler, rules=None, subdomain_map=None):
            self.subdomain_map = subdomain_map or {}
            self.delegate_handler = delegate_handler
            super(SubdomainApplicationRouter, self).__init__(application, rules)

        def select_handler(self, request):
            return ArgusContentHandler if is_argus_action(request) else self.delegate_handler

        def find_handler(self, request, **kwargs):
            m = cookieless_regexp.match(request.host)
            if m is not None:
                subdomain = m.group(1)
            else:
                logging.warning("got request to naydex without a subdomain")
                return self.forbidden(request)

            subdomain_configs = self.subdomain_map.get(subdomain)
            if not subdomain_configs:
                logging.warning("unknown subdomain received: {}".format(subdomain))
                return self.forbidden(request)
            user_agent_data, device_type = get_ua_data_and_device(request.headers.get('User-Agent'))
            checked_ip_headers = check_ip_headers_are_internal(system_config.YANDEX_NETS, request.headers)
            config = select_configs(subdomain_configs, device_type, exp_id=request.headers.get(system_config.EXPERIMENT_ID_HEADER),
                                    is_internal=ip_headers_is_internal(checked_ip_headers))
            return self.application.get_handler_delegate(request, self.select_handler(request), target_kwargs=dict(
                config=config, pipeline=pipeline,  ua_data=user_agent_data, checked_ip_headers=checked_ip_headers))

        def forbidden(self, request):
            return self.application.get_handler_delegate(request, ForbiddenHandler)

    # initial HTTPClient configuration, going to be updated at WorkerApplication.update_configs
    configure_http_client(configs)

    token_map = defaultdict(dict)
    naydex_subdomain_map = defaultdict(dict)
    for key, config in configs.iteritems():
        for token in config.PARTNER_TOKENS:
            token_map[token][key] = config
            naydex_subdomain_map[config.name.replace('.', '-').replace('_', '-')][key] = config

    autoredirect_config = configs.get(system_config.AUTOREDIRECT_SERVICE_ID) or configs.get(system_config.AUTOREDIRECT_KEY)
    if autoredirect_config is not None:
        for item in autoredirect_config.webmaster_data.keys():
            naydex_subdomain_map[item.replace('.', '-').replace('_', '-')][system_config.AUTOREDIRECT_KEY] = autoredirect_config

    app = WorkerApplication()

    static_mon_router = StaticMonApplicationRouter(app)
    token_crypt_content_router = TokenApplicationRouter(app, CryptContentHandler, [Rule(AnyMatches(), app.wildcard_router)], token_map=token_map)
    token_router = TokenApplicationRouter(app, CryHandler, [Rule(AnyMatches(), app.wildcard_router)], token_map=token_map)
    cookie_router = TokenApplicationRouter(app, CookieHandler, [Rule(AnyMatches(), app.wildcard_router)], token_map=token_map)
    crypt_url_router = TokenApplicationRouter(app, CryptUrlHandler, [Rule(AnyMatches(), app.wildcard_router)], token_map=token_map)
    naydex_router = SubdomainApplicationRouter(app, CryHandler, [Rule(AnyMatches(), app.wildcard_router)], subdomain_map=naydex_subdomain_map)
    generate_cookie_router = GenerateCookieApplicationRouter(app)

    if ENV_TYPE in ['testing', 'development']:
        app.add_handlers("workercontrol", [(PathMatches(r'/v1/control/.*'), WorkerControlHandler),
                                           (PathMatches(r'/control/update_bypass_uids'), WorkerControlHandler),
                                           (PathMatches(r'/control/update_experiment_config'), WorkerControlHandler)])

    app.add_handlers(system_config.COOKIE_MATCHER_DOMAIN_RE, [(AnyMatches(), CookieCryptHandler)])

    app.add_handlers(system_config.COOKIELESS_HOST_RE, [(AnyMatches(), naydex_router)])

    app.add_handlers(system_config.DETECT_LIB_HOST_RE,  # vhost routing for detect lib
                     [(UriMatcher(re_escape(lib_path + '?pid=') + '\\w+'), static_mon_router) for lib_path in system_config.DETECT_LIB_PATHS] +
                     [(PathMatches(re_escape(lib_path)), token_router) for lib_path in system_config.DETECT_LIB_PATHS] +
                     [(PathMatches(r'/cookie_of_the_day'), cookie_router)] +
                     [(PathMatches(r'/get_crypted_url'), crypt_url_router)])

    app.add_handlers(r'.*',
                     [(UriMatcher(re_escape('/static/optional.js?pid=') + '[\\w\\.]+' + '&script_key=' + '\\S+'), generate_cookie_router)] +
                     [(PathMatches(r'/crypt_content'), token_crypt_content_router)] +
                     [(AnyMatches(), token_router)])  # default vhost routing

    app.token_crypt_content_router = token_crypt_content_router
    app.token_router = token_router
    app.cookie_router = cookie_router
    app.crypt_url_router = crypt_url_router
    app.naydex_router = naydex_router
    app.configs_cache = configs

    return app


def make_service_app(children, tvm_client, redis_sentinel):
    return ServiceApplication([
        (r'/ping', PingHandler),
        (r'/v1/configs/', ConfigsHandler),
        (r'/v1/control/reload_configs', ControlHandler, dict(children=children)),
        (r'/control/update_bypass_uids', ControlHandler, dict(children=children)),
        (r'/control/update_experiment_config', ControlHandler, dict(children=children)),
        (r'/v1/control/reload_configs_cache', CacheControlHandler),
        (r'/metrics', MetricsHandler),
        (r'/system_metrics', SystemMetricsHandler),
        (r'/static_cache', StaticCacheHandler, dict(redis_sentinel=redis_sentinel)),
        (r'/argus_cache', ArgusCacheHandler, dict(redis_sentinel=redis_sentinel)),
        (r'.*', ServiceHandler),
    ], tvm_client)
