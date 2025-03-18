import aiohttp
import logging
import json

from tenacity import (
    AsyncRetrying,
    RetryError,
    stop_after_attempt,
    wait_exponential,
    retry_if_exception_type,
    after_log,
    TryAgain,
)

log = logging.getLogger(__name__)


class TVM2AsyncBase(object):
    RETRY_CODES = {
        500,
        502,
        499,
    }

    ERROR_CODES = {
        400,
        401,
    }

    def get_headers(self):
        return {}

    def get_session(self):
        timeout = aiohttp.ClientTimeout(total=5)
        return aiohttp.ClientSession(
            timeout=timeout,
            headers=self.get_headers(),
        )

    async def get_service_ticket(self, destination, **kwargs):
        tickets = await self.get_service_tickets(destination, **kwargs)
        return tickets.get(destination)

    async def parse_response(self, response):
        if response.status in self.RETRY_CODES:
            raise TryAgain()
        elif response.status in self.ERROR_CODES:
            body = await response.text()
            log.warning(
                f'got error: {body}'
            )
            response = {}

        else:
            response = await response.text()
            response = json.loads(response)
            if 'error' in response:
                log.warning(
                    f'got error: {response["error"]}, debug: {response.get("logging_string")}'
                )
                response = {}
        return response

    async def make_request(self, path, method='get', params=None, data=None, headers=None):
        url = '{}/{}'.format(self.api_url, path)
        response_data = {}
        async with self.get_session() as session:
            try:
                async for attempt in AsyncRetrying(
                    stop=stop_after_attempt(self.retries),
                    retry=retry_if_exception_type(TryAgain),
                    wait=wait_exponential(multiplier=0.1),
                    after=after_log(log, logging.WARNING)

                ):
                    with attempt:
                        async with getattr(session, method)(
                            url=url,
                            params=params,
                            data=data,
                            headers=headers,
                        ) as response:
                            response_data = await self.parse_response(response)
            except RetryError:
                # ретраи кончились
                pass

        return response_data
