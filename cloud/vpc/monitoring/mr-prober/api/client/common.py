from typing import TypeVar, Type, Optional, Union, Dict
from urllib.parse import urljoin

import pydantic
import requests
import requests.exceptions

Model = TypeVar("Model", bound=pydantic.BaseModel)


class BaseUrlSession(requests.Session):
    """
    A Session with a URL that all requests will use as a base.
    Based on request_toolbelt.BaseUrlSession
    """

    base_url: Optional[str] = None

    def __init__(self, base_url: Optional[str] = None):
        if base_url:
            self.base_url = base_url
        super().__init__()

    def request(self, method: str, url: str, *args, **kwargs):
        if self.base_url is not None:
            url = urljoin(self.base_url, url)
        return super().request(
            method, url, *args, **kwargs
        )


class ModelBasedHttpClient(requests.Session):
    """
    Wrapper around `requests.Session` with response model validation.

    You can call `client.get(...)`, `client.post(...)` etc as usual. These calls without additional arguments
    check exclusively whether status code is not 4XX and not 5XX.

    But you can pass `..., expected_status_code=status.HTTP_412_PRECONDITION_FAILED` if you expect
    specific status code.

    You can also pass `..., response_model=models.ClusterListResponse` if you're expecting this object in the response.
    In this case response will be JSON-decoded and parsed into a model object. If `response_model` is not passed,
    standard `requests.Response` is returned.

    Also you can use `..., json=models.AnyModel()` as a shortcut for `..., json=models.AnyModel().dict()`.

    Examples of usage:

    1. response = client.get("/clusters", response_model=models.ClusterListResponse)

    2. response = client.post(
        "/recipes",
        json=models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="/recipes/1",
            name="meeseeks",
            description="I'm Mr. Meeseeks! Look at me!"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse,
    )

    3. response = client.post(
        "/clusters",
        json=models.CreateClusterRequest(recipe_id=1, manually_created=True, variables=[]),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
        response_model=models.ErrorResponse,
    )
    """

    def __init__(self, underlying_client: Optional[requests.Session] = None):
        self._client = underlying_client or requests.Session()
        super().__init__()

    def get(
        self,
        *args,
        response_model: Optional[Type[Model]] = None,
        expected_status_code: Optional[int] = None,
        **kwargs
    ) -> Union[requests.Response, Model]:
        self._prepare_request(kwargs)
        response = self._client.get(*args, **kwargs)
        return self._process_response(response, response_model, expected_status_code)

    def post(
        self,
        *args,
        response_model: Optional[Type[Model]] = None,
        expected_status_code: Optional[int] = None,
        **kwargs
    ) -> Union[requests.Response, Model]:
        self._prepare_request(kwargs)
        response = self._client.post(*args, **kwargs)
        return self._process_response(response, response_model, expected_status_code)

    def put(
        self,
        *args,
        response_model: Optional[Type[Model]] = None,
        expected_status_code: Optional[int] = None,
        **kwargs
    ) -> Union[requests.Response, Model]:
        self._prepare_request(kwargs)
        response = self._client.put(*args, **kwargs)
        return self._process_response(response, response_model, expected_status_code)

    def patch(
        self,
        *args,
        response_model: Optional[Type[Model]] = None,
        expected_status_code: Optional[int] = None,
        **kwargs
    ) -> Union[requests.Response, Model]:
        self._prepare_request(kwargs)
        response = self._client.patch(*args, **kwargs)
        return self._process_response(response, response_model, expected_status_code)

    def delete(
        self,
        *args,
        response_model: Optional[Type[Model]] = None,
        expected_status_code: Optional[int] = None,
        **kwargs
    ) -> Union[requests.Response, Model]:
        self._prepare_request(kwargs)
        response = self._client.delete(*args, **kwargs)
        return self._process_response(response, response_model, expected_status_code)

    def _prepare_request(self, kwargs: Dict):
        if "json" in kwargs and isinstance(kwargs["json"], pydantic.BaseModel):
            kwargs["json"] = kwargs["json"].dict()

    def _process_response(
        self, response: requests.Response,
        response_model: Optional[Type[Model]] = None, expected_status_code: Optional[int] = None,
    ) -> Union[requests.Response, Model]:
        if expected_status_code is None:
            assert response.ok, self._get_unexpected_status_code_message(response)
        else:
            assert response.status_code == expected_status_code, self._get_unexpected_status_code_message(response)

        if response_model is None:
            return response

        try:
            return response_model.parse_obj(response.json())
        except pydantic.ValidationError as e:
            raise MrProberClientError(f"Can not parse response as {response_model.__name__}: {e}\n\n"
                                      f"API response is {response.json()}")
        except Exception as e:
            raise MrProberClientError(f"Can not parse response: {e}\n\n"
                                      f"HTTP status code is {response.status_code}. API response is {response.text}")

    @staticmethod
    def _get_unexpected_status_code_message(response):
        try:
            response_content = response.json()
        except requests.exceptions.JSONDecodeError:
            try:
                response_content = response.text
            except ValueError:
                response_content = response.content
        return f"server returned HTTP status code {response.status_code} with following content: {response_content}"

    @property
    def headers(self):
        return self._client.headers

    @headers.setter
    def headers(self, value):
        self._client.headers = value

    @property
    def verify(self):
        return self._client.verify

    @verify.setter
    def verify(self, value):
        self._client.verify = value

    def __repr__(self) -> str:
        return repr(self._client)


class MrProberClientError(Exception):
    pass
