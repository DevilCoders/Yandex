import io
from typing import List, Optional, Any

import api.models
import settings
from .common import ModelBasedHttpClient, BaseUrlSession

DEFAULT_ENDPOINT = f"http://{settings.HOST}:{settings.PORT}"


class ProbersClient:
    def __init__(self, underlying_client: ModelBasedHttpClient):
        self._client = underlying_client

    def list(self) -> List[api.models.Prober]:
        return self._client.get("/probers", response_model=api.models.ProberListResponse).probers

    def get(self, prober_id: int) -> api.models.Prober:
        return self._client.get(f"/probers/{prober_id}", response_model=api.models.ProberResponse).prober

    def create(self, request: api.models.CreateProberRequest) -> api.models.Prober:
        return self._client.post(f"/probers", json=request, response_model=api.models.CreateProberResponse).prober

    def update(self, prober_id: int, request: api.models.UpdateProberRequest) -> api.models.Prober:
        return self._client.put(
            f"/probers/{prober_id}", json=request, response_model=api.models.UpdateProberResponse
        ).prober

    def delete(self, prober_id: int):
        self._client.delete(f"/probers/{prober_id}", response_model=api.models.SuccessResponse)

    def copy(self, prober_id: int) -> api.models.Prober:
        return self._client.post(
            f"/probers/copy",
            json=api.models.CopyProberRequest(prober_id=prober_id),
            response_model=api.models.CreateProberResponse
        ).prober

    def add_file(self, prober_id: int, request: api.models.CreateProberFileRequest) -> api.models.ProberFile:
        return self._client.post(
            f"/probers/{prober_id}/files", json=request, response_model=api.models.CreateProberFileResponse
        ).file

    def delete_file(self, prober_id: int, file_id: int):
        self._client.delete(f"/probers/{prober_id}/files/{file_id}", response_model=api.models.SuccessResponse)

    def update_file(
        self, prober_id: int, file_id: int, request: api.models.UpdateProberFileRequest
    ) -> api.models.ProberFile:
        return self._client.put(
            f"/probers/{prober_id}/files/{file_id}", json=request,
            response_model=api.models.UpdateProberFileResponse
        ).file

    def get_file_content(self, prober_id: int, file_id: int) -> bytes:
        return self._client.get(f"/probers/{prober_id}/files/{file_id}/content").content

    def upload_file_content(self, prober_id: int, file_id: int, content: bytes):
        self._client.put(
            f"/probers/{prober_id}/files/{file_id}/content",
            files={"content": ("", io.BytesIO(content))},
            response_model=api.models.UpdateProberFileResponse,
        )

    def add_config(self, prober_id: int, request: api.models.CreateProberConfigRequest) -> api.models.ProberConfig:
        return self._client.post(
            f"/probers/{prober_id}/configs", json=request, response_model=api.models.CreateProberConfigResponse
        ).config

    def delete_config(self, prober_id: int, config_id: int):
        self._client.delete(f"/probers/{prober_id}/configs/{config_id}", response_model=api.models.SuccessResponse)

    def copy_configs(self, prober_id: int, from_prober_id: int):
        self._client.post(
            f"/probers/{prober_id}/configs/copy", json=api.models.CopyOrMoveProberConfigsRequest(
                prober_id=from_prober_id
            ), response_model=api.models.SuccessResponse
        )

    def move_configs(self, prober_id: int, from_prober_id: int):
        self._client.post(
            f"/probers/{prober_id}/configs/move", json=api.models.CopyOrMoveProberConfigsRequest(
                prober_id=from_prober_id
            ), response_model=api.models.SuccessResponse
        )

    def update_config(
        self, prober_id: int, config_id: int, request: api.models.UpdateProberConfigRequest
    ) -> api.models.ProberConfig:
        return self._client.put(
            f"/probers/{prober_id}/configs/{config_id}", json=request,
            response_model=api.models.UpdateProberConfigResponse
        ).config


class ClustersClient:
    def __init__(self, underlying_client: ModelBasedHttpClient):
        self._client = underlying_client

    def list(self) -> List[api.models.Cluster]:
        return self._client.get("/clusters", response_model=api.models.ClusterListResponse).clusters

    def get(self, cluster_id: int) -> api.models.Cluster:
        return self._client.get(f"/clusters/{cluster_id}", response_model=api.models.ClusterResponse).cluster

    def create(self, request: api.models.CreateClusterRequest) -> api.models.Cluster:
        return self._client.post(f"/clusters", json=request, response_model=api.models.CreateClusterResponse).cluster

    def update(self, cluster_id: int, request: api.models.UpdateClusterRequest) -> api.models.Cluster:
        return self._client.put(
            f"/clusters/{cluster_id}", json=request, response_model=api.models.UpdateClusterResponse
        ).cluster

    def delete(self, cluster_id: int):
        self._client.delete(f"/clusters/{cluster_id}", response_model=api.models.SuccessResponse)

    def update_variable_value(self, cluster_id: int, variable_id: int, value: Any):
        self._client.put(
            f"/clusters/{cluster_id}/variables/{variable_id}",
            json=api.models.UpdateClusterVariableRequest(value=value),
            response_model=api.models.UpdateClusterVariableResponse
        )


class RecipesClient:
    def __init__(self, underlying_client: ModelBasedHttpClient):
        self._client = underlying_client

    def list(self) -> List[api.models.ClusterRecipe]:
        return self._client.get("/recipes", response_model=api.models.ClusterRecipeListResponse).recipes

    def get(self, recipe_id: int) -> api.models.ClusterRecipe:
        return self._client.get(f"/recipes/{recipe_id}", response_model=api.models.ClusterRecipeResponse).recipe

    def create(self, request: api.models.CreateClusterRecipeRequest) -> api.models.ClusterRecipe:
        return self._client.post(
            f"/recipes", json=request, response_model=api.models.CreateClusterRecipeResponse
        ).recipe

    def update(self, recipe_id: int, request: api.models.UpdateClusterRecipeRequest) -> api.models.ClusterRecipe:
        return self._client.put(
            f"/recipes/{recipe_id}", json=request, response_model=api.models.UpdateClusterRecipeResponse
        ).recipe

    def delete(self, recipe_id: int):
        self._client.delete(f"/recipes/{recipe_id}", response_model=api.models.SuccessResponse)

    def add_file(
        self, recipe_id: int, request: api.models.CreateClusterRecipeFileRequest
    ) -> api.models.ClusterRecipeFile:
        return self._client.post(
            f"/recipes/{recipe_id}/files", json=request, response_model=api.models.CreateClusterRecipeFileResponse
        ).file

    def delete_file(self, recipe_id: int, file_id: int):
        self._client.delete(f"/recipes/{recipe_id}/files/{file_id}", response_model=api.models.SuccessResponse)

    def get_file_content(self, recipe_id: int, file_id: int) -> bytes:
        return self._client.get(f"/recipes/{recipe_id}/files/{file_id}/content").content

    def upload_file_content(self, recipe_id: int, file_id: int, content: bytes):
        self._client.put(
            f"/recipes/{recipe_id}/files/{file_id}/content",
            files={"content": ("", io.BytesIO(content))},
            response_model=api.models.UpdateClusterRecipeFileResponse,
        )


class MrProberApiClient:
    def __init__(
        self, endpoint: Optional[str] = None, api_key: Optional[str] = None,
        cache_control: Optional[str] = None,
        underlying_client: Optional[ModelBasedHttpClient] = None
    ):
        if endpoint is None and underlying_client is None:
            raise ValueError("MrProberApiClient: both endpoint and underlying_client are None, specify one of them")
        if endpoint is not None and underlying_client is not None:
            raise ValueError(
                "MrProberApiClient: both endpoint and underlying_client are specified, use only one of them"
            )

        if endpoint is not None:
            self._endpoint = endpoint
            http_client = ModelBasedHttpClient(BaseUrlSession(endpoint))
            if api_key is not None:
                http_client.headers["X-API-KEY"] = api_key
            if cache_control is not None:
                http_client.headers["Cache-Control"] = cache_control
            if settings.API_ROOT_CERTIFICATES_PATH.exists():
                http_client.verify = settings.API_ROOT_CERTIFICATES_PATH.absolute().as_posix()
        else:
            self._endpoint = repr(underlying_client)
            http_client = underlying_client

        self.probers = ProbersClient(http_client)
        self.clusters = ClustersClient(http_client)
        self.recipes = RecipesClient(http_client)

    def __repr__(self) -> str:
        return self._endpoint
