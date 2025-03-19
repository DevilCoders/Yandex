import asyncio
import logging
from typing import Callable
from typing import Dict
from typing import Optional
from typing import Union

import aiohttp

# from cloud.dwh.utils.log import mask_sensitive_fields
# from cloud.dwh.utils.misc import ellipsis_string
# from cloud.dwh.utils.misc import format_url

LOG = logging.getLogger(__name__)

AiohttpMethod = Callable[..., aiohttp.ClientResponse]
Response = Union[dict, str]


class AsyncHttpClient:
    DEFAULT_USER_AGENT = 'YC DWH'

    def __init__(self, endpoint: str, token: Optional[str] = None, session_manager: Optional['_SessionManager'] = None, *, user_agent: str = None, debug: Optional[bool] = None):
        self._endpoint = endpoint
        self._token = token

        self._user_agent = self.DEFAULT_USER_AGENT if user_agent is None else user_agent
        self._debug = LOG.level < logging.INFO if debug is None else debug

        self.__session_manager = session_manager

    async def _get_base_headers(self) -> Dict[str, str]:
        return {
            'User-Agent': self._user_agent,
            'Accept': 'application/json',
        }

    async def _get_auth_headers(self) -> Dict[str, str]:
        if self._token is None:
            return {}

        return {
            'Authorization': f'OAuth {self._token}'
        }

    async def _get_headers(self) -> Dict[str, str]:
        return {**await self._get_base_headers(), **await self._get_auth_headers()}

    def _get_url(self, path: str) -> str:
        return f'{self._endpoint}{path}'

    async def request(self,
                      method: AiohttpMethod,
                      path: str,
                      params: Optional[dict] = None,
                      body: Optional[dict] = None,
                      headers: Optional[dict] = None,
                      extra_headers: Optional[dict] = None,
                      raise_for_status: bool = True,
                      **kwargs) -> Response:
        """
        :param method: request method
        :param path: url path without endpoint
        :param params: query params
        :param body: body params
        :param headers: headers to replace default headers
        :param extra_headers: extra headers that are added to default headers (see _get_headers method)
        :param raise_for_status: if it is True, error will be raised on non 2xx statuses
        :param kwargs: other aiohttp.ClientSession parameters
        :return: dict or if response is not json, it will be raw text
        """

        url = self._get_url(path)

        if headers is None:
            headers = await self._get_headers()

        if extra_headers is not None:
            headers.update(extra_headers)

        # if self._debug:
        #     log_message = 'API call: %s %s'
        #     log_args = [method.__name__.upper(), format_url(url, query_params=params)]
        #
        #     if body:
        #         log_message += ' %s'
        #         log_args.append(mask_sensitive_fields(body))
        #
        #     LOG.debug(log_message, *log_args)

        async with self.__session_manager as session:
            async with await method(
                session,
                url=url,
                headers=headers,
                params=params,
                json=body,
                **kwargs
            ) as response:  # type: aiohttp.ClientResponse
                try:
                    result = await response.json()
                except aiohttp.client_exceptions.ContentTypeError:
                    result = await response.text()

                if raise_for_status and not response.ok:
                    LOG.error('API call failed with status code %s and response: %s', response.status, result)
                    LOG.error(f'params - {params}, body - {body}')
                    response.raise_for_status()

                # if self._debug:
                #     log_result = mask_sensitive_fields(result)
                #     log_result = ellipsis_string(str(log_result), 10000)
                #     LOG.debug('API call: Result with status code %s: %s.', response.status, log_result)

                return result

    async def get(self,
                  path: str,
                  params: Optional[dict] = None,
                  body: Optional[dict] = None,
                  headers: Optional[dict] = None,
                  extra_headers: Optional[dict] = None,
                  **kwargs) -> Response:
        return await self.request(aiohttp.ClientSession.get, path=path, params=params, body=body, headers=headers, extra_headers=extra_headers, **kwargs)

    async def post(self,
                   path: str,
                   params: Optional[dict] = None,
                   body: Optional[dict] = None,
                   headers: Optional[dict] = None,
                   extra_headers: Optional[dict] = None,
                   **kwargs) -> Response:
        return await self.request(aiohttp.ClientSession.post, path=path, params=params, body=body, headers=headers, extra_headers=extra_headers, **kwargs)

    async def put(self,
                  path: str,
                  params: Optional[dict] = None,
                  body: Optional[dict] = None,
                  headers: Optional[dict] = None,
                  extra_headers: Optional[dict] = None,
                  **kwargs) -> Response:
        return await self.request(aiohttp.ClientSession.put, path=path, params=params, body=body, headers=headers, extra_headers=extra_headers, **kwargs)

    async def patch(self,
                    path: str,
                    params: Optional[dict] = None,
                    body: Optional[dict] = None,
                    headers: Optional[dict] = None,
                    extra_headers: Optional[dict] = None,
                    **kwargs) -> Response:
        return await self.request(aiohttp.ClientSession.patch, path=path, params=params, body=body, headers=headers, extra_headers=extra_headers, **kwargs)

    async def delete(self,
                     path: str,
                     params: Optional[dict] = None,
                     body: Optional[dict] = None,
                     headers: Optional[dict] = None,
                     extra_headers: Optional[dict] = None,
                     **kwargs) -> Response:
        return await self.request(aiohttp.ClientSession.delete, path=path, params=params, body=body, headers=headers, extra_headers=extra_headers, **kwargs)


class _SessionManager:
    _auto_sessions: Dict[str, aiohttp.ClientSession]
    _user_session: Optional[aiohttp.ClientSession] = None

    def __init__(self):
        self._auto_sessions = {}

    async def set_user_session(self, session: Optional[aiohttp.ClientSession]):
        self._user_session = session

    async def close_user_session(self):
        self._user_session and await self._user_session.close()

    def __bool__(self):
        return bool(self._user_session)

    async def __aenter__(self):
        session = self._user_session
        if session is None:
            session = aiohttp.ClientSession()
            self._auto_sessions[asyncio.current_task().get_name()] = session

        return session

    async def __aexit__(self, exc_type, exc, tb):
        auto_session = self._auto_sessions.get(asyncio.current_task().get_name())
        auto_session and await auto_session.close()


class AsyncApiClient:
    def __init__(self, endpoint: str, token: str, *, user_agent: str = None, debug: Optional[bool] = None, **_):
        self.__session_manager = _SessionManager()
        self._http_client = AsyncHttpClient(endpoint=endpoint, token=token, session_manager=self.__session_manager, user_agent=user_agent, debug=debug)

    async def __aenter__(self):
        session = aiohttp.ClientSession()
        await self.__session_manager.set_user_session(session)

        return self

    async def __aexit__(self, exc_type, exc, tb):
        await self.__session_manager.close_user_session()
