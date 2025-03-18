import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
# DO NOT DELETE THIS!!!
# this will update sys.path which is required for import
import gencfg

import xmlrpclib
from flask import request, send_from_directory
from flask import make_response as flask_make_response
from flask_restful import Api, Resource
import traceback
import logging
import datetime
import blackbox
import Cookie
from recordtype import recordtype
import socket
import time
import uuid
import gc
import urllib
import __main__
import copy
import random

from core.db import CURDB
import core.exceptions
from config import *
from gaux.aux_utils import mkdirp, plain_obj_str_to_unicode, plain_obj_unicode_to_str, \
    run_command, setup_logger_logfile, OAuthTransport
from gaux.aux_staff import get_possible_group_owners
from gaux.aux_repo import get_tags_by_tag_pattern
import service_ctl
from dbman import TagsDBManager, TrunkDBManager, UnstableDBManager
from core.settings import SETTINGS
from core.argparse.parser import ArgumentParserExt
import ujson

from gaux.aux_performance import perf_timer

# TODO: move to config
SUPERUSERS = ['kimkim', 'okats', 'sereglond']

DNS_CACHE_TIME = 300


class AuthorizerBase(object):
    def is_super_user(self, login):
        return login in SUPERUSERS


class SitemapPage(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        return dict(resources=self.api.get_resources())


class PrecalcCachesPage(Resource):
    @classmethod
    def init(cls, api):
        cls.db = api.db
        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        # generate group caches
        self.db.precalc_caches()

        # generate misc caches
        get_possible_group_owners()

        return {}


class CheckBranchPage(Resource):
    @classmethod
    def init(cls, api):
        cls.db = api.db
        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        self.db.fast_check()
        return {}


class CheckBranchPageExt(CheckBranchPage):
    pass


# for debug purposes
class RunGCPage(Resource):
    def gencfg_authorize_post(self, login, request):
        del login, request
        return True

    def post(self):
        gc.collect()
        return {}


class TagsListPage(Resource):
    # getting all tags is very slow, so we have to cache it
    # FIXME: non-thread safe
    CACHED = None
    CACHED_AT = 0

    @classmethod
    def init(cls, api):
        cls.db = api.db

        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        now = time.time()
        if now - TagsListPage.CACHED_AT > 20:
            tags = list(reversed(get_tags_by_tag_pattern(TAG_PATTERN, self.db.get_repo())))

            try:  # filter out unreleased tags if have ones
                sandbox_proxy = xmlrpclib.ServerProxy(SETTINGS.services.sandbox.xmlrpc.url, allow_none=True,
                                                      transport=OAuthTransport('invalid_oauth_token'))

                old_timeout = socket.getdefaulttimeout()
                socket.setdefaulttimeout(0.5)
                try:
                    last_released_resource = sandbox_proxy.list_resources(
                        {'limit': 1, 'resource_type': 'CONFIG_GENERATOR', 'state': 'READY',
                         'all_attrs': {'released': 'stable'}})[0]
                finally:
                    socket.setdefaulttimeout(old_timeout)

                last_released_tag = last_released_resource["attrs"]["tag"]

                if tags[0] != last_released_tag:
                    tags.pop(0)
            except:
                pass

            TagsListPage.CACHED = {
                "tags": tags,
                "displayed_tags": tags,
            }
            TagsListPage.CACHED_AT = now

        return copy.copy(TagsListPage.CACHED)


class TestingInfo(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        executable = os.path.abspath(__main__.__file__)
        assert (os.path.basename(executable) == "main.py")
        backend_type = os.path.basename(os.path.dirname(executable))
        backend_type = {
            "api_backend": "api",
            "wbe_backend": "wbe",
        }[backend_type]

        return {
            "db_type": self.api.options.db,
            "backend_type": backend_type,
            "commit_enabled": self.api.commit_enabled(),
        }


class BranchTestingInfo(TestingInfo):
    def get(self):
        result = TestingInfo.init(self.api).get(self)
        result["db_path"] = self.api.db.get_path()
        result["db_commit"] = self.api.db.get_repo().get_last_commit_id()
        return result


class LastCommit(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        return cls

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        change_statuses = self.api.db.get_repo().get_changed_file_statuses(self.api.db.get_path())
        modified_count = len(change_statuses["modified"])
        unversioned_count = len(filter(lambda x: x != "cache", change_statuses["unversioned"]))

        result = {
            "last_commit": self.api.db.get_repo().get_last_commit_id(),
            "modified": (modified_count + unversioned_count) > 0
        }

        return result


class StaticFilesPage(Resource):
    STATIC_FILES_DIR = os.path.join(os.path.dirname(__main__.__file__), "static")

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self, path):
        # alternative:
        #   Flask(__name__, static_folder='api/static', static_url_path='/static')
        #   (but cannot do std gencfg authorization here)
        return send_from_directory(STATIC_FILES_DIR, path)


AuthnInfo = recordtype('AuthnInfo', [
    'login',
    'firstname',
    'lastname',
    'user_ip',
    'source_ip',
],
                       default=None)

PerformanceInfo = recordtype('ProfileInfo', [
    'start_time',
    'end_time',
    'total_time',
],
                             default=None)


class Authner(object):
    def __init__(self, api):
        # stupid bureaucracy
        # these values uses blackbox to choose appropreate server auth
        self.api = api
        self.blackbox = blackbox.JsonBlackbox(url=SETTINGS.services.blackbox.rest.url)

    def _get_user_ip(self, request):
        # get user ip
        forwarded_list = request.headers.get("X-Forwarded-For")
        if forwarded_list is None:
            user_ip = request.remote_addr
        else:
            user_ip = forwarded_list.split(',')[0].strip()
        return user_ip

    def auth_unauthenticated(self, request):
        return AuthnInfo(login=None, firstname=None, lastname=None, user_ip=self._get_user_ip(request),
                         source_ip=request.remote_addr)

    def __auth_user(self, request):
        result = self.auth_unauthenticated(request)

        cookies = request.headers.get("Cookie")
        if cookies is None:
            return result
        cookies = Cookie.SimpleCookie(str(cookies))

        if self.api.options.use_yandex_login_cookie:
            """
                Session_id is not always available. When frontend work in http mode (not https), it does not receive Session_id and sessionid2 cookies
            """
            yandex_login = cookies.get("yandex_login")
            if yandex_login is None:
                return result

            result.login = yandex_login.value
            result.firstname = yandex_login.value
            result.lastname = ''
            return result
        else:
            # get Session_id cookie
            Session_id = cookies.get("Session_id")
            if Session_id is None:
                return result
            Session_id = Session_id.value

            try:
                bb_reply = self.blackbox.sessionid(
                    sessionid=Session_id,
                    userip=self._get_user_ip(request),
                    host="yandex-team.ru",
                    dbfields=[],
                )
            except Exception as err:
                raise Exception("Blackbox access error: %s" % str(err))

            # print 'REPLY', bb_reply

            error = bb_reply.get(u'error')
            if error is None:
                raise Exception("Incorrect blackbox reply")
            if error != u"OK":
                raise Exception("Authentication failed: %s" % error)

            login = bb_reply.get(u'login')
            if login is None:
                raise Exception("Incorrect blackbox reply")
            login = str(login)  # ignore conversion issues, none expected
            result.login = login
            result.firstname = login
            result.lastname = ''
            return result

    def auth_user(self, request):
        try:
            return True, self.__auth_user(request)
        except Exception as err:
            return False, Exception("Login authentication failed: %s" % err)
        except:
            return False, Exception("Login authentication failed: Undefined error")


class Authzer(object):
    def __init__(self, api):
        self.api = api

    def auth_user(self, login, request):
        """
            Check user permissions on request.

            :param login: user login
            :param request: object with all request data
            :return: pair of (status, reason) where status is True or False, and reason is explanation, why user can not execute query
        """

        if request.routing_exception:
            return True, None

        key = str(request.url_rule)
        assert (key in self.api.resources), "Key <%s> not found in  <%s>" % (key, self.api.resources)
        cls = self.api.resources[key]
        instance = cls()

        # Solutions design is motivated by paranoya. That is why
        # for each method you should create distinct
        #   gencfg_authorize_<method> function
        # Because it is easy to forget about authorization rules
        # when you add resource/method
        method_name = 'gencfg_authorize_%s' % request.method.lower()
        method = getattr(instance, method_name, None)
        if method is None:
            return False, "Method <%s> is not allowed for request <%s>" % (request.method.lower(), key)

        # FIXME: no exception handler here
        response = method(login, request)
        if not isinstance(response, tuple):
            response = response, "Unknown reason"
        return response


class WebApiBase(Api):
    class GencfgNotFoundError(RuntimeError):
        def __init__(self, msg):
            self.msg = msg

    class GencfgForbiddenError(RuntimeError):
        def __init__(self, msg):
            self.msg = msg

    class GencfgAuthenticationRequiredError(RuntimeError):
        def __init__(self, msg):
            self.msg = msg

    def __init__(self, *args, **kwargs):
        options = kwargs['options']
        del kwargs['options']
        self.options = options

        if self.options.db == "trunk":
            self.db_man = TrunkDBManager(self, self.options.trunk_db_path)
        elif self.options.db == "unstable":
            self.db_man = UnstableDBManager(self, self.options.trunk_db_path)
        elif self.options.db == "tags":
            self.db_man = TagsDBManager(self, max_simultaneous_dbs=self.options.max_tags)
        else:
            raise Exception("Unsupported --db type \"%s\"" % self.options.db)

        # do not use out of request context as it may not exist
        self.db = CURDB

        app = args[0]
        Api.__init__(self, *args, **kwargs)
        self.app = app

        # This code rewrites function to convert result json to what we send to network
        # We do not have to transform data, because it is already dupmed json
        @self.representation('application/json')
        def output_json(data, code, headers=None):
            resp = flask_make_response(data, code)
            resp.headers.extend(headers or {})
            return resp

        def _before_request(*args, **kwargs):
            return self.before_request()

        def _after_request(*args, **kwargs):
            return self.after_request(*args, **kwargs)

        self.app.before_request(_before_request)
        self.app.after_request(_after_request)

        # do not need default log stdout handler
        if options.verbose_level == 0:
            del self.app.logger.handlers[:]

        if not self.options.disable_log:
            info_log_path = os.path.join(SETTINGS.backend.logging.logdir,
                                         SETTINGS.backend.logging.logtpl % dict(port=self.options.port))
            info_log_path = os.path.abspath(info_log_path)
            setup_logger_logfile(self.app.logger, info_log_path, logging.INFO)

            debug_log_path = os.path.join(SETTINGS.backend.logging.logdir,
                                          SETTINGS.backend.logging.dbglogtpl % dict(port=self.options.port))
            debug_log_path = os.path.abspath(debug_log_path)
            setup_logger_logfile(self.app.logger, debug_log_path, logging.DEBUG)

            self.app.logger.setLevel(logging.DEBUG)

            if not os.path.exists(debug_log_path):
                open(debug_log_path, 'w').close()

        self.authner = Authner(self)
        self.authzer = Authzer(self)

        from setup_page import PageSetup
        self.resources = {}

        self.add_resource(SitemapPage.init(self), '/sitemap')
        self.add_resource(PageSetup.init(self), '/setup')
        self.add_resource(RunGCPage, '/run_gc')
        self.add_resource(CheckBranchPage.init(self), '/check')
        self.add_resource(TestingInfo.init(self), '/testing_info')

        # build static dir and create corresponding resource if not exists
        base_dir = os.path.dirname(__main__.__file__)
        build_static_path = os.path.join(base_dir, "build_static.sh")
        if os.path.exists(build_static_path):
            run_command([build_static_path])
        if os.path.exists(StaticFilesPage.STATIC_FILES_DIR):
            self.add_resource(StaticFilesPage, '/static/<path:filename>')

        self.add_branch_resource(BranchTestingInfo.init(self), "/testing_info")
        self.add_branch_resource(PrecalcCachesPage.init(self), '/precalc_caches')
        self.add_branch_resource(CheckBranchPageExt.init(self), '/check')
        if self.options.db in ["trunk", "unstable"]:
            self.add_branch_resource(LastCommit.init(self), '/repo/last_commit')
            self.add_branch_resource(TagsListPage.init(self), '/tags')

        self.before_request_success = True

        self.dns_cache = {}

        self.uuid = options.uuid

        self.server_name = socket.gethostname()

        self.external_url_protocol = self.options.external_url_protocol
        self.external_url_host = self.options.external_url_host if self.options.external_url_host else socket.gethostname()
        self.external_url_port = self.options.external_url_port if self.options.external_url_port else self.options.port

        # semi-debug stuff
        self.queued_reqs = []
        self.processed_reqs = []

        # start updater thread if auto update is enabled
        if self.options.enable_auto_update:
            self.db_man.db_updater_thread.start()

        self.app.logger.info('[%s]\t[CORE]\t[LISTENER]\tListening started' % (self.make_time_str()))

    def gen_base_url(self):
        if self.external_url_port and self.external_url_port != 80:
            port = ':%s' % self.external_url_port
        else:
            port = ''
        return "%s://%s%s/%s" % (self.external_url_protocol, self.external_url_host, port, self.options.db)

    def get_server_name(self):
        return self.server_name

    def handle_error(self, e):
        # change return code and print error string
        if not isinstance(e, BaseException):
            return super(WebApiBase, self).handle_error(e)

        tb = traceback.format_exc(sys.exc_info()[2])
        # noinspection PyUnusedLocal
        error_msg = "Undefined error"
        # noinspection PyUnusedLocal
        error_traceback = ""
        if isinstance(e, WebApiBase.GencfgNotFoundError):
            http_code = 404
            error_msg = e.msg
            error_traceback = tb
        elif type(e) is WebApiBase.GencfgForbiddenError:
            http_code = 403
            error_msg = 'Forbidden: <%s>' % e.msg
            error_traceback = ""
        elif type(e) is WebApiBase.GencfgAuthenticationRequiredError:
            # TODO: add Location header
            # https://passport.yandex-team.ru/passport?retpath=https://sandbox.yandex-team.ru/sandbox/
            http_code = 302
            error_msg = 'Authentication required: <%s>' % e.msg
            error_traceback = ""
        elif type(e) in (Exception, core.exceptions.TValidateCardNodeError):
            # in all gencfg code we throw Exception, so let's assume
            # that it means that we found some problem related with user request
            http_code = 400
            self.app.logger.info(tb)
            error_msg = str(e)
            error_traceback = tb
        else:
            # all other types are system types, let's assume that
            # there is some problem in code
            http_code = 500
            self.app.logger.error(tb)
            error_msg = str(e)
            error_traceback = tb

        return self.make_response(
            dict(
                error=error_msg,
                error_traceback=error_traceback,
            ),
            http_code)

    # setup logging
    def _setup_file_log(self, path, lvl):
        mkdirp(os.path.dirname(path))
        if not os.path.exists(path):
            open(path, 'w').close()
            os.chmod(path, 0777)
        handler = logging.FileHandler(path)
        handler.setLevel(lvl)
        self.app.logger.addHandler(handler)

    def _resolve_addr(self, addr):
        now = time.time()
        name, req_time = self.dns_cache[addr] if addr in self.dns_cache else (None, None)
        if req_time and req_time + DNS_CACHE_TIME / 2 + random.randrange(0, DNS_CACHE_TIME / 2) > now:
            return name
        try:
            name = socket.gethostbyaddr(addr)[0]
        except socket.error:
            name = addr
        self.dns_cache[addr] = (name, now)
        return name

    def _make_ip_str(self, request):
        if getattr(request, "yauth", None) and request.yauth.source_ip != request.yauth.user_ip:
            ip_str = "%s (%s)" % (
                self._resolve_addr(request.yauth.user_ip),
                self._resolve_addr(request.remote_addr))
        else:
            if request.remote_addr is not None:
                ip_str = str(self._resolve_addr(request.remote_addr))
            else:
                ip_str = "None"
        return ip_str

    def _make_login_str(self, request):
        if getattr(request, "yauth", None) is None or request.yauth.login is None:
            login_str = "???"
        else:
            login_str = str(request.yauth.login)
        return login_str

    def make_time_str(self):
        return datetime.datetime.now().strftime('%d/%b/%y %H:%M:%S.%f')

    def before_request(self):
        try:
            return self.do_before_request()
        except:
            self.before_request_success = False
            raise

    def do_before_request(self):
        # TODO: use not "request", "g" - also thread-local flask var!
        request.response_str = ""
        request.yauth = None
        request.perf_info = None
        request.branch = None
        request.short_uuid = None

        headers_strings = []
        for key, value in request.headers:
            if key == 'Cookie':
                value = '*' * 6
            if value:
                headers_strings.append('%s: %s' % (key, value))

        try:
            perf_info = PerformanceInfo()
            perf_info.start_time = datetime.datetime.now()
            request.perf_info = perf_info
            request.perf_info.total_time = 0

            request.short_uuid = str(uuid.uuid4())[:8]
            self.queued_reqs.append(request.short_uuid)

            if request.environ['PATH_INFO'] == '/trunk/groups/HEARTBEAT_SERVER_GUEST':
                return self.make_response({
                    "group": "HEARTBEAT_SERVER_GUEST",
                    "owners": ["ekilimchuk","mocksoul","oplachkin","torkve"],
                    "master": None,
                    "hosts": ["man1-3652-10410.vm.search.yandex.net","man1-5817-10410.vm.search.yandex.net","sas1-2024-10410.vm.search.yandex.net","sas1-4142-10410.vm.search.yandex.net","vla1-0668-10410.vm.search.yandex.net","vla1-1220-10410.vm.search.yandex.net"],
                    "debug_info": {
                            "backend_host":"minos.search.yandex.net",
                            "backend_type":"trunk",
                            "backend_port":13579,
                            "branch":"trunk",
                            "backend_disk_path":"/ssd/home/kimkim/work/gencfg.svn/web_shared"
                    }
                }, 200)


            # short uuid we use to match request start record with request end record in log
            self.app.logger.info('[%s]\t[%s]\t[REQ]\t[ACCEPTED]\t[%s]\t[%s]\t"%s %s"' % (
                self.make_time_str(), request.short_uuid, self._make_login_str(request), self._make_ip_str(request),
                request.method, request.url))
            self.app.logger.info('[%s]\t[%s]\t[QUEUE]\t[QUEUED_REQS]\t%s' % (self.make_time_str(), request.short_uuid, ' '.join(self.queued_reqs)))

            self.app.logger.debug('\n'.join(headers_strings))
            self.app.logger.debug('<[[\n%s\n]]>>' % request.data)

            # Simplest throttling.
            if len(self.queued_reqs) > 100:
                self.before_request_success = False
                return self.make_response({"error": "Service unavailable due to overload", "url": request.url}, 503)

            try:
                request.json
            except Exception:
                raise Exception("Cannot parse JSON body")
            try:
                # FIXME: in order to update request.json we have to update internal
                # field _cached_json as json is @property
                request._cached_json = plain_obj_unicode_to_str(request.json)
            except Exception:
                raise

            # special handler for /check
            if str(request.url_rule) == "/check":
                if not request.view_args:
                    request.view_args = {}
                if self.options.db == "trunk":
                    request.view_args['branch'] = "trunk"
                elif self.options.db == "tags":
                    request.view_args['branch'] = "recent"
                else:
                    pass

            if request.view_args and 'branch' in request.view_args:
                request.branch = request.view_args['branch']
                del request.view_args['branch']
                if request.branch == "recent":
                    request.branch = self.db_man.get_recent_branch()
            else:
                request.branch = None

            if request.branch:
                self.db_man.begin_request(request)

                self.app.logger.info('[%s]\t[%s]\t[REQ]\t[PREPARED]' % (self.make_time_str(), request.short_uuid))

            # authentication
            if self.options.allow_auth:
                isOK, auth_result = self.authner.auth_user(request)
                if not isOK:
                    self.app.logger.error(str(auth_result))
                    auth_result = self.authner.auth_unauthenticated(request)
                request.yauth = auth_result
            else:
                request.yauth = self.authner.auth_unauthenticated(request)

            # authorization
            if self.options.allow_auth:
                isOK, reason = self.authzer.auth_user(request.yauth.login, request)
                if not isOK:
                    if request.yauth.login is None:
                        raise WebApiBase.GencfgAuthenticationRequiredError(reason)
                    else:
                        raise WebApiBase.GencfgForbiddenError(reason)
            else:
                pass

        finally:
            self.processed_reqs.append(request.short_uuid)
            self.app.logger.info('[%s]\t[%s]\t[REQ]\t[STARTED]' % (self.make_time_str(), request.short_uuid))
            self.app.logger.info('[%s]\t[%s]\t[QUEUE]\t[PROCESSED_REQS]\t%s' % (self.make_time_str(), request.short_uuid, ' '.join(self.processed_reqs)))

    def after_request(self, response):
        # WARNING: this function is called even if we had exception.
        # And sometimes we have exceptions in before_request and not all data
        # required in this function is initialized.
        # This can lead us to second exception and second exception is what users see when
        # he should see the first one. This is not good.
        #
        # So let's do special check

        try:
            self.do_after_request(response)
        except:
            if self.before_request_success:
                raise
            else:
                pass
        return response

    def do_after_request(self, response):
        try:
            self.queued_reqs.remove(request.short_uuid)
            try:
                self.processed_reqs.remove(request.short_uuid)
            except ValueError:
                pass

            if request.branch:
                self.db_man.end_request(request.branch)

            request.perf_info.end_time = datetime.datetime.now()
            request.perf_info.total_time = (request.perf_info.end_time - request.perf_info.start_time).total_seconds()

            # str to unicode conversion is done in make_response function
            return response
        finally:
            total_time_str = '%.3f' % request.perf_info.total_time
            self.app.logger.info('[%s]\t[%s]\t[REQ]\t[FINISHED]\t[%s]\t[%s]\t[%s sec]\t%s' % (
                self.make_time_str(), request.short_uuid, self._make_login_str(request), self._make_ip_str(request),
                total_time_str,
                response.status))
            self.app.logger.debug('<[[\n%s\n]]>>' % request.response_str)

    def make_response(self, data, *args, **kwargs):
        if isinstance(data, dict):
            data["debug_info"] = dict(
                backend_host=self.server_name,
                backend_port=self.options.port,
                backend_type=self.options.db,
                backend_disk_path=os.path.dirname(__file__),
                branch=request.branch,
            )

        # convert str to unicode
        if isinstance(data, dict):
            data = plain_obj_str_to_unicode(data)

        # save object before it will be (possibly) compressed
        if isinstance(data, dict):
            request.response_str = ujson.dumps(data)
        else:
            request.response_str = str(data)

        flask_response = super(WebApiBase, self).make_response(request.response_str, *args, **kwargs)
        flask_response.headers['Access-Control-Allow-Origin'] = '*'

        return flask_response

    def add_absolute_resource(self, page_class, url):
        self.add_resource(page_class, url)

    def add_control_resource(self, page_class, url):
        # so can be one of:
        #   /tags/ctl/...
        #   /trunk/ctl/...
        #   /unstable/ctl/...
        self.add_resource(page_class, "/%s/ctl" % self.options.db + url)

    def add_branch_resource(self, page_class, url):
        if self.options.db == "tags":
            prefix = "/tags"
        else:
            prefix = ""
        # so can be one of:
        #   /tags/<tags-name>/...
        #   /trunk/...
        #   /unstable/...
        self.add_resource(page_class, prefix + '/<string:branch>' + url)

    def add_resource(self, page_class, url):
        from setup_page import shutdown_mode
        if shutdown_mode:
            return
        self.resources[url] = page_class

        super(WebApiBase, self).add_resource(page_class, url)

    def get_resources(self):
        return self.resources.keys()

    @staticmethod
    def global_run_service(main_func):
        # we use it to return application specific return codes
        # particularly exit code with socket error

        retcode = 0
        try:
            main_func()
        except socket.error as err:
            sys.stderr.write(traceback.format_exc(err))
            retcode = SERVICE_EXIT_CODE_SOCKET_ERR
        except Exception as err:
            if str(err).find('[Errno 98] Address already in use') != -1:
                retcode = SERVICE_EXIT_CODE_SOCKET_ERR
            else:
                retcode = SERVICE_EXIT_CODE_OTHER_ERR
            sys.stderr.write(traceback.format_exc(err))
        except:
            retcode = SERVICE_EXIT_CODE_OTHER_ERR

        return retcode

    @perf_timer
    def commit(self, message, request, reset_changes_on_fail=False):
        if not self.commit_enabled():
            return

        header_commit_message = request.headers.get("X-Gencfg-Commit-Message")
        if header_commit_message is not None and header_commit_message != "":
            message = urllib.unquote(header_commit_message)
        if not message:
            message = "<empty commit message>"

        custom_commit_author = request.headers.get("X-Gencfg-Commit-Author")

        if request:
            login = request.yauth.login
            firstname = request.yauth.firstname
            lastname = request.yauth.lastname
        else:
            login = None
            firstname = None
            lastname = None
        if login and firstname is not None and lastname is not None:
            author_msg = "%s %s <%s@yandex-team.ru>" % (
                firstname.encode('utf8'),
                lastname.encode('utf8'),
                login)
        else:
            author_msg = None


        changed, commits_discarded = self.db.commit("%s%s (commited by %s@)" % (self.COMMIT_PREFIX, message, custom_commit_author if custom_commit_author else login),
                                                    author=author_msg, reset_changes_on_fail=reset_changes_on_fail)

        if changed:
            service_ctl.shutdown()

        if commits_discarded:
            raise Exception("Failed to process the request: the changes has been lost during DB data merge.")

    def commit_enabled(self):
        return self.options.allow_commit


def build_base_argparser(description):
    parser = ArgumentParserExt(description=description, prog="PROG")
    parser.add_argument("-p", "--port", dest="port", type=int, default=None,
                        help="service port")
    parser.add_argument("--host", dest="host", type=str, default="::",
                        help="service host")
    parser.add_argument("--production", dest="production", action="store_true", default=False,
                        help="run in production mode.")
    parser.add_argument("--testing", dest="testing", action="store_true", default=False,
                        help="run in testing mode.")
    parser.add_argument("--no-auth", dest="allow_auth", action="store_false", default=True,
                        help="use authorization")
    parser.add_argument("--use-yandex-login-cookie", action="store_true", default=False,
                        help="Use yandex_login instead of sessionid to get user login in authentification")
    parser.add_argument("--uuid", dest="uuid", default=str(uuid.uuid1()),
                        help="uuid for process")
    parser.add_argument("--no-commit", dest="allow_commit", action="store_false", default=True,
                        help="do not commit DB changes (all changes are local)")
    parser.add_argument("--external-url-host", dest="external_url_host",
                        help="external url hostname")
    parser.add_argument("--external-url-port", dest="external_url_port", type=int, default=None,
                        help="external url port")
    parser.add_argument("--external-url-protocol", dest="external_url_protocol", default="http",
                        help="external url protocol")
    parser.add_argument("--db", dest="db", type=str, default="trunk", choices=["trunk", "unstable", "tags"],
                        help="db for work")
    parser.add_argument("--trunk-db-path", dest="trunk_db_path", type=str, default=None,
                        help="Optional. Current db path (if --db == 'trunk')")
    parser.add_argument("--max-tags", dest="max_tags", type=int, default=1,
                        help="Optional. maximum number of tags opened simultaneously")
    parser.add_argument("--single-thread", dest="threaded", action="store_false", default=True,
                        help="Optional. Run in single thread")
    parser.add_argument("-v", "--verbose-level", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")
    parser.add_argument("--disable-log", action="store_true", default=False,
                        help="Optional. Disable logging (to speedup testing requests, for example")
    parser.add_argument("--enable-cache", action="store_true", default=False,
                        help="Optional. Enable requests caching in mongo")
    parser.add_argument("--enable-auto-update", action="store_true", default=False,
                        help="Optional. Auto update db without service restart")

    return parser
