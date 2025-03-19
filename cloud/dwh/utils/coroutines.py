import asyncio
from typing import Any
from typing import AsyncGenerator
from typing import Coroutine
from typing import List


def run_sync(coro: Coroutine) -> Any:
    loop = asyncio.get_event_loop()
    return loop.run_until_complete(coro)


async def async_generator_to_list(agen: AsyncGenerator[Any, None]) -> List[Any]:
    return [i async for i in agen]


def parallel_limiter(limit: int):
    def decorator(func):
        semaphore = None

        async def wrapper(*args, **kwargs):
            nonlocal semaphore
            if semaphore is None:
                semaphore = asyncio.Semaphore(limit)

            async with semaphore:
                return await func(*args, **kwargs)

        return wrapper

    return decorator
