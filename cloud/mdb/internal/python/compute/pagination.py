from typing import Any, Callable, Iterator, NamedTuple, Generator


class ComputeRequest(NamedTuple):
    page_token: str


class ComputeResponse(NamedTuple):
    next_page_token: str
    resources: Iterator[Any]


def paginate(method: Callable[[ComputeRequest], ComputeResponse], request: Any) -> Generator[Any, None, None]:
    page_token = ''
    while True:
        request.page_token = page_token
        resp = method(request)
        for resource in resp.resources:
            yield resource
        page_token = resp.next_page_token
        if not page_token:
            break
