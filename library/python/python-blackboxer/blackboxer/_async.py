import aiohttp
import tenacity

from blackboxer.blackboxer import BlackboxMixin
from blackboxer.environment import URL
from blackboxer.exceptions import FieldRequiredError, HTTPError, TemporaryError, BlackboxError
from blackboxer.utils import choose_first_not_none


class AsyncBlackbox(BlackboxMixin):
    def __init__(self, url=URL, timeout=1, retries=3, session=None, **kwargs):
        """ Асинхронный клиент для доступа к blackbox


        :param str url: url до ЧЯ, по умолчанию урл определяется yenv
        :param int timeout: таймаут на чтение и запись
        :param int retries: количество повторных обращений при возникновении ошибок
        :param requests.Session session: сессия для доступа к ЧЯ
        """
        self.url = url
        self.timeout = timeout
        self.retries = retries
        self.session = session or aiohttp.ClientSession()

        self.backoff_factor = kwargs.get("backoff_factor", 0.3)
        self.status_forcelist = kwargs.get("status_forcelist", (500, 502, 504))
        self.format = kwargs.get("format", "json")

        self._retry = tenacity.AsyncRetrying(
            wait=tenacity.wait_exponential(multiplier=self.backoff_factor),
            stop=tenacity.stop_after_attempt(self.retries + 1),
            retry=tenacity.retry_if_exception(
                lambda e: isinstance(e, aiohttp.ClientResponseError)
                and e.status in self.status_forcelist
            ),
            reraise=True,
        ).wraps(
            self._arequest
        )

    async def _close(self):
        await self.session.close()

    async def _arequest(self, **kwargs):
        resp = await self.session.request(**kwargs)
        resp.raise_for_status()
        return resp

    async def _do_req(self, http_method, **kwargs):
        """

        :param str|unicode http_method:
        :rtype: requests.Response
        """
        try:
            return await self._retry(
                method=http_method, url=self.url, timeout=self.timeout, **kwargs
            )
        except aiohttp.ClientConnectionError as e:
            raise ConnectionError(e)
        except aiohttp.ClientResponseError as e:
            raise HTTPError(e)

    async def _req(self, http_method, **kwargs):
        """

        :param str|unicode http_method:
        :param kwargs:
        :rtype dict:
        """
        response = await self._do_req(http_method, **kwargs)
        parsed = self._parse_response(await response.text())

        try:
            self._raise_error(parsed)
        except TemporaryError:
            # TODO сделать опциональный перезапуск
            raise

        return parsed

    async def _make_request(self, http_method, payload, **kwargs):
        """

        :param str|unicode http_method:
        :param dict payload:
        :param kwargs:
        :rtype dict:
        """
        if http_method == "GET":
            return await self._req("GET", params=payload, **kwargs)
        elif http_method == "POST":
            return await self._req("POST", data=payload, **kwargs)
        else:
            raise BlackboxError("Unknown http method %s" % http_method)

    async def _get(
        self, blackbox_method, required_fields=None, exclusive_fields=None, headers=None, **extra
    ):
        return await self.custom_method(
            "GET", blackbox_method, required_fields, exclusive_fields, headers, **extra
        )

    async def _post(
        self, blackbox_method, required_fields=None, exclusive_fields=None, headers=None, **extra
    ):
        return await self.custom_method(
            "POST", blackbox_method, required_fields, exclusive_fields, headers, **extra
        )

    async def custom_method(
        self,
        http_method,
        blackbox_method,
        required_fields=None,
        exclusive_fields=None,
        headers=None,
        **extra
    ):
        """ Вызов метода `blackbox_method` в ЧЯ

        если нужного метода в клиенте еще нет
        можно легко реализовать свой метод

        :param str|unicode http_method: HTTP метод для вызова (GET, POST)
        :param str|unicode blackbox_method: метод в blackbox (прим. userinfo)
        :param required_fields: обязательные параметры метода
        :param exclusive_fields: обязательные взаимоисключающие параметры
        :param extra: необязательные параметры
        :rtype: dict
        """
        payload = {"method": blackbox_method, "format": self.format}

        payload.update(required_fields)

        if exclusive_fields:
            exclusive = choose_first_not_none(exclusive_fields)
            if not exclusive:
                raise FieldRequiredError("choose from %s" % exclusive_fields.keys())
            payload.update(exclusive)

        payload.update(extra)

        self._prepare_payload(payload)

        return await self._make_request(http_method, payload, headers=(headers or {}))

    async def userinfo(self, userip, uid=None, suid=None, login=None, headers=None, **extra):
        """ Метод userinfo возвращает сведения о пользователе.

        Основное назначение метода — доступ к данным системы авторизации

        https://doc.yandex-team.ru/blackbox/reference/MethodUserInfo.xml

        обязательные параметры:
        :param str userip:

        взаимоисключающие параметры
        :param str uid:
        :param str suid:
        :param str login:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = "userinfo"

        required = {
            "userip": userip,
        }

        exclusive = {"uid": uid, "suid": suid, "login": login}

        return await self._get(method_name, required, exclusive, headers, **extra)

    async def login(self, userip, password, authtype, uid=None, login=None, headers=None, **extra):
        """Метод login проверяет пароль для учетной записи, идентифицированной логином или UID.

        При успешной аутентификации метод также может возвращать сведения об учетной записи,
        запрошенные в базе данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/MethodLogin.xml

        обязательные параметры:
        :param str userip:
        :param str password:
        :param str authtype:

        взаимоисключающие параметры
        :param str uid:
        :param str login:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = "login"

        required = {"userip": userip, "password": password, "authtype": authtype}

        exclusive = {"uid": uid, "login": login}

        return await self._post(method_name, required, exclusive, headers, **extra)

    async def sessionid(self, userip, sessionid, host, headers=None, **extra):
        """Метод sessionid проверяет валидность кук Session_id и sessionid2.

        При успешной аутентификации метод также может возвращать сведения
        о соответствующей учетной записи из базы данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/MethodSessionID.xml

        обязательные параметры
        :param str userip:
        :param str sessionid:
        :param str host:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = "sessionid"

        required = {"sessionid": sessionid, "userip": userip, "host": host}

        return await self._get(method_name, required, headers=headers, **extra)

    async def oauth(self, userip, oauth_token, headers=None, **extra):
        """ Метод oauth проверяет OAuth-токен, выданный сервисом oauth.yandex.ru.

        Если токен валиден, метод также может возвращать запрошенные сведения
        о соответствующей учетной записи из базы данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/method-oauth.xml

        обязательные параметры
        :param userip:
        :param oauth_token:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = "oauth"

        required = {
            "userip": userip,
        }

        headers = dict(headers) if headers else {}
        headers["Authorization"] = "OAuth {}".format(oauth_token)

        return await self._get(method_name, required, headers=headers, **extra)

    async def lcookie(self, l, headers=None, **extra):
        """ Метод lcookie извлекает данные из переданной куки L: UID, логин и время выставления куки.

        Чтобы вызывать метод, необходимо запросить грант allow_l_cookie в рассылке passport-admin@.

        https://doc.yandex-team.ru/blackbox/reference/method-lcookie.xml

        обязательные параметры
        :param str|unicode l:

        :param extra:
        :rtype: dict
        """
        method_name = "lcookie"

        required = {"l": l}

        return await self._get(method_name, required, headers=headers, **extra)

    async def checkip(self, ip, nets, headers=None, **extra):
        """ Метод checkip проверяет, принадлежит ли IP-адрес пользователя сетям Яндекса.

        https://doc.yandex-team.ru/blackbox/reference/method-checkip.xml

        обязательные параметры
        :param str|unicode ip:
        :param str|unicode nets:

        :param extra:
        :rtype: dict
        """
        method_name = "checkip"

        required = {"ip": ip, "nets": nets}

        return await self._get(method_name, required, headers=headers)
