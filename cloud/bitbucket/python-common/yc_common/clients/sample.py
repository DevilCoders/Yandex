from typing import Iterable

from schematics.types import ListType, ModelType

from yc_common import config
from yc_common.clients.api import ApiClient
from yc_common.constants import ServiceNames
from yc_common.models import Model, StringType
from yc_requests.credentials import YandexCloudCredentials


class Sample(Model):
    id = StringType(required=True)
    project_id = StringType(required=False)
    name = StringType(required=True)


class SampleClient:
    def __init__(self, url, credentials: YandexCloudCredentials):
        self.__client = ApiClient(url + "public/v1", service_name=ServiceNames.SAMPLE, credentials=credentials)

    def list_samples(self, project_id) -> Iterable[Sample]:
        class Result(Model):
            result = ListType(ModelType(Sample), required=True)
        return iter(self.__client.get("/projects/{}/samples".format(project_id), model=Result).result)

    def get_sample(self, project_id, sample_id) -> Sample:
        return self.__client.get("/projects/{}/sample/{}".format(project_id, sample_id), model=Sample)

    def create_sample(self, project_id, sample) -> Sample:
        return self.__client.post("/projects/{}/samples".format(project_id), sample, model=Sample)

    def update_sample(self, project_id, sample_id,  sample) -> Sample:
        return self.__client.put("/projects/{}/sample/{}".format(project_id, sample_id), sample, model=Sample)

    def delete_sample(self, project_id, sample_id):
        return self.__client.delete("/projects/{}/sample/{}".format(project_id, sample_id))


class SampleEndpointConfig(Model):
    url = StringType(required=True)


def get_sample_client(auth: YandexCloudCredentials) -> SampleClient:
    return SampleClient(config.get_value("endpoints.sample", model=SampleEndpointConfig).url, auth)
