# -*- coding: utf8 -*-
import re2
from monotonic import monotonic
from urllib import unquote
from urlparse import parse_qsl, urlparse

from antiadblock.cryprox.cryprox.common.tools.misc import duration_usec, remove_headers
from antiadblock.cryprox.cryprox.common.tools.url import classify_url, UrlClass
from antiadblock.cryprox.cryprox.service import action
from antiadblock.cryprox.cryprox.common.cryptobody import IMAGE_EXTENTIONS_RE
from antiadblock.cryprox.cryprox.config.system import AUTOREDIRECT_QARGS, AUTOREDIRECT_FIND_TEMPLATE_RE, FETCH_URL_HEADER_NAME, INAPP_REQUEST_URL_PATHS
from antiadblock.cryprox.cryprox.config.static_config import MATCH_YANDEX_DOMAIN_WITH_TLD_RE

REPLACE_SPEC_CHARACTERS = {'&amp;': '&', '&quot;': '"', '&nbsp;': '%C2%A0', '&lt;': '<', '&gt;': '>'}
SPEC_CHARS_RE = re2.compile('|'.join(REPLACE_SPEC_CHARACTERS.keys()))


def fixup_url(func):
    def payload(self, *args, **kwargs):
        url = self.request.url

        if url.path.startswith('/an/') and MATCH_YANDEX_DOMAIN_WITH_TLD_RE.match(url.hostname):  # ANTIADB-2779
            url = url._replace(netloc='an.yandex.ru', path=url.path[3:])

        if url.fragment:  # https://st.yandex-team.ru/ANTIADB-1315 удаляем часть ссылки после якоря
            url = url._replace(fragment='')

        if url.query:
            new_url_query = url.query

            for char in set(SPEC_CHARS_RE.findall(new_url_query)):  # https://st.yandex-team.ru/ANTIADB-1310 обработка спецсимволов HTML
                new_url_query = new_url_query.replace(char, REPLACE_SPEC_CHARACTERS[char])
            url = url._replace(query=new_url_query)
        # https://st.yandex-team.ru/ANTIADB-2626 change url for Ads SDK (inapp)
        if not self.is_crypted_url:
            _url = self.request.headers.get(FETCH_URL_HEADER_NAME)
            if _url is not None and url.path in INAPP_REQUEST_URL_PATHS:
                remove_headers(self.request.headers, FETCH_URL_HEADER_NAME)
                # нужно раздекодить каунтовый урл в инаппе https://st.yandex-team.ru/ANTIADB-3130#62e008fa9e233b370a142471
                if ("/count" in _url or "/rtbcount" in _url) and "%" in _url:
                    _url = unquote(_url)
                url = urlparse(_url)

        # https://st.yandex-team.ru/ANTIADB-2803, https://st.yandex-team.ru/ANTIADB-3061
        if url.netloc == "yastatic.net" and url.path == "/pcode/adfox/loader_adb.js":
            url = url._replace(netloc="an.yandex.ru", path="/system/context_adb.js")

        self.request.url = url

        func(self, *args, **kwargs)

    return payload


def fixup_scheme(func):
    def payload(self, *args, **kwargs):
        """
        Working like a proxy. Switch the scheme of url

        :param decrypted: if this url was obtained from crypted url
        """
        if self.request.headers.get('X-Forwarded-Proto', None) not in ['http', 'https', None]:
            self.logger.warning('X-Forwarded-Proto header validation failed due to invalid value: {}'.format(self.request.headers['X-Forwarded-Proto']))
            self.set_status(400)
            self.write('Bad X-Forwarded-Proto header value\n')
            self.finish()
            return
        # X-Forwarded-Proto scheme should use for non-crypted links. For crypted links use inner protocol scheme instead
        if UrlClass.PARTNER in self.request.tags and not self.is_crypted_url or self.request.url.scheme == '':
            scheme = self.request.headers.get('X-Forwarded-Proto', self.original_url.scheme)
            self.request.url = self.request.url._replace(scheme=scheme)

        # https://st.yandex-team.ru/ANTIADB-1968
        if UrlClass.AUTOREDIRECT_SCRIPT in self.request.tags:
            old_qargs = parse_qsl(self.request.url.query)
            request_qargs = {key: value for key, value in old_qargs if key in AUTOREDIRECT_QARGS}
            scheme = request_qargs.get("proto", "https")
            netloc = request_qargs.get("domain")
            path, query = request_qargs.get("path"), ""
            if not netloc or not path or scheme not in ['http', 'https']:
                self.logger.warning('Url validation failed due to invalid value: domain - {}, path - {}, proto - {}'.format(netloc, path, scheme))
                self.set_status(400)
                self.write('Bad url\n')
                self.finish()
                return
            # TODO: провалидировать реферер запроса и домен, откуда будет загружен ресурс
            if '?' in path:
                path, query = path.split("?", 1)
            # https://st.yandex-team.ru/ANTIADB-2113
            replace_param = ""
            match = AUTOREDIRECT_FIND_TEMPLATE_RE.search(path)
            if match:
                replace_param = match.group(1)
                for key, value in old_qargs:
                    if key == replace_param:
                        replace_value = value
                        break
                else:
                    self.logger.warning('Url validation failed due to invalid replace template: path - {}, replace_param - {}'.format(path, replace_param))
                    self.set_status(400)
                    self.write('Bad url\n')
                    self.finish()
                    return
                path = path.replace('${{{}}}'.format(replace_param), replace_value)

            param = "&".join(["{}={}".format(k, v) for k, v in old_qargs if k not in AUTOREDIRECT_QARGS + (replace_param,)])
            if param:
                query = param + (('&' + query) if query else '')
            self.request.url = self.request.url._replace(scheme=scheme, netloc=netloc, path=path, query=query)

        func(self, *args, **kwargs)

    return payload


def remove_cookies(func):
    def payload(self, *args, **kwargs):
        for cookie in self.config.EXCLUDE_COOKIE_FORWARD:
            if cookie in self.request.cookies:
                del self.request.cookies[cookie]
        # Удаление кук из self.request.cookies не удаляет их из заголовка `Cookie`, так что приходится формировать его заново
        self.request.headers['Cookie'] = '; '.join(
            ['{}={}'.format(cookie[0], cookie[1].value) for cookie in self.request.cookies.items()])
        func(self, *args, **kwargs)

    return payload


def redirect(func):
    def payload(self, *arg, **kwargs):
        url = self.request.url.geturl()
        if self.config.client_redirect_url_re_match is not None and self.config.client_redirect_url_re_match.match(url) is not None:
            self.logger.info(None, action=action.REDIRECT, url=url)
            self.metrics.increase_counter(action.REDIRECT)
            self.redirect(url)
        else:
            func(self, *arg, **kwargs)

    return payload


def accel_redirect(func):
    def payload(self, *args, **kwargs):
        accel_redirect_prefix = None
        url = self.request.url.geturl()

        if UrlClass.PARTNER in self.request.tags and \
                ((self.config.PARTNER_ACCEL_REDIRECT_ENABLED and IMAGE_EXTENTIONS_RE.match(self.request.url.path)) or
                 (self.config.partner_accel_url_re is not None and self.config.partner_accel_url_re.search(url) is not None)):
            accel_redirect_prefix = self.config.PARTNER_ACCEL_REDIRECT_PREFIX
        elif self.config.accel_url_re is not None and self.config.accel_url_re.search(url) is not None:
            accel_redirect_prefix = self.config.ACCEL_REDIRECT_PREFIX

        if accel_redirect_prefix:
            self.logger.info(None, action=action.ACCELREDIRECT, url=accel_redirect_prefix + url)
            self.metrics.increase_counter(action.ACCELREDIRECT)
            self.add_header('X-Accel-Redirect', accel_redirect_prefix + url)
            self.finish()
        else:
            func(self, *args, **kwargs)

    return payload


def chkaccess(func):
    def payload(self, *args, **kwargs):
        url = self.request.url
        urlstr = url.geturl()
        (self.request.tags, duration) = duration_usec(classify_url, self.config, urlstr)

        if any([tag in self.config.forbidden_tags for tag in self.request.tags]):
            self.request.tags.append(UrlClass.DENY)
        self.logger.request_tags += [tag.name for tag in self.request.tags]
        self.logger.debug(None, action=action.CHKACCESS, duration=duration, url=urlstr)
        self.metrics.increase(action.CHKACCESS, duration)

        if UrlClass.DENY in self.request.tags:
            self.logger.warning('Forbidden by cryprox: DENY', action=action.CHKACCESS, duration=duration, url=urlstr)
            self.set_status(403)
            self.write('Forbidden\n')
            self.finish()
            return

        func(self, *args, **kwargs)

    return payload


def logged_action(action_name=None):
    """
    Calculate func operation time, log its result and increment metric counter
    :param action_name: action name to log
    """
    def inner_decorator(func):
        def payload(self, *args, **kwargs):
            start = monotonic()
            try:
                result = func(self, *args, **kwargs)
            finally:
                end = monotonic()
                duration = int((end - start) * 1000000)
                self.logger.info(None, action=action_name, duration=duration)
                self.metrics.increase(action_name, duration=duration)
            return result
        return payload

    return inner_decorator
