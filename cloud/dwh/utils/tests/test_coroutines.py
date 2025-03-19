import asyncio
from types import AsyncGeneratorType

import pytest

from cloud.dwh.utils.coroutines import async_generator_to_list
from cloud.dwh.utils.coroutines import parallel_limiter
from cloud.dwh.utils.coroutines import run_sync


def test_run_sync():
    async def coro():
        await asyncio.sleep(.1)
        return 1, 2, 3

    result = run_sync(coro())
    assert result == (1, 2, 3)


@pytest.mark.asyncio
async def test_async_generator_to_list():
    async def coro():
        yield 1
        yield 2

    result = coro()
    assert isinstance(result, AsyncGeneratorType)

    result = await async_generator_to_list(coro())

    assert result == [1, 2]


@pytest.mark.asyncio
async def test_parallel_limiter_decorator():
    @parallel_limiter(1)
    async def coro():
        return 1

    result = await coro()

    assert result == 1


@pytest.mark.asyncio
async def test_parallel_limiter_func():
    async def coro():
        return 1

    new_coro = parallel_limiter(1)(coro)

    result = await new_coro()

    assert result == 1
