"""TeamCity client."""

import cgi

import requests
from requests.auth import HTTPBasicAuth

from yc_common import config, logging
from yc_common.clients.api import ApiClient, YcClientInternalError
from yc_common.exceptions import Error
from yc_common.models import Model, ListType, ModelType, StringType, IntType, DictType

log = logging.get_logger(__name__)


class QueuedBuild(Model):
    id = StringType(required=True)
    state = StringType(required=True)
    build_type_id = StringType(serialized_name="buildTypeId", required=True)
    branch_name = StringType(serialized_name="branchName", required=True)


class BuildSummary(QueuedBuild):
    status = StringType()
    status_text = StringType(serialized_name="statusText")


class BuildType(Model):
    id = StringType(required=True)


class Build(BuildSummary):
    build_type = ModelType(BuildType, serialized_name="buildType", required=True)


class Artifact(Model):
    name = StringType(required=True)
    href = StringType(required=True)
    size = IntType(required=True)
    mtime = StringType(serialized_name="modificationTime", required=True)
    content = DictType(StringType, required=True)


class BuildState:
    QUEUED = "queued"
    RUNNING = "running"
    FINISHED = "finished"


class BuildStatus:
    SUCCESS = "SUCCESS"
    FAILURE = "FAILURE"


class TeamcityError(Error):
    def __init__(self, message, *args):
        super().__init__("TeamCity: " + message, *args)


class TeamcityClient(ApiClient):
    def __init__(self, url, user, password):
        super().__init__(url.rstrip("/") + "/httpAuth/app/rest", auth=HTTPBasicAuth(user, password), extra_headers={
            "Accept": "application/json"
        })

    def get_build(self, build_id):
        return self.get("/builds/id:" + build_id, retry_temporary_errors=True, model=Build)

    def get_builds(self, model=BuildSummary, **kwargs):
        class Result(Model):
            build = ListType(ModelType(model), default=list, required=True)

        return self.get("/builds", params=kwargs, retry_temporary_errors=True, model=Result).build

    def get_build_queue(self, model=QueuedBuild, **kwargs):
        class Result(Model):
            build = ListType(ModelType(model), default=list, required=True)

        return self.get("/buildQueue", params=kwargs, retry_temporary_errors=True, model=Result).build

    def trigger_build(self, build_type, branch_name):
        return self.post("/buildQueue", Build.new(build_type=BuildType.new(id=build_type), branch_name=branch_name),
                         model=Build)

    def get_artifact(self, build_id, file_name, stream=False):
        return self.get("/builds/id:{build_id}/artifacts/content/{file_name}".format(
            build_id=build_id, file_name=file_name), stream=stream, retry_temporary_errors=True)

    def list_artifacts(self, build_id):
        class Result(Model):
            file = ListType(ModelType(Artifact), required=True)

        response = self.get("/builds/id:{build_id}/artifacts/children".format(
            build_id=build_id), retry_temporary_errors=True, model=Result)
        return response.file

    def _parse_response(self, response, model: Model=None):
        content_type, type_options = cgi.parse_header(response.headers.get("Content-Type", ""))

        if response.status_code != requests.codes.ok:
            if 500 <= response.status_code < 600:
                raise YcClientInternalError("TeamCity returned an error with {} status code for {}.",
                                            response.status_code, response.url)
            elif content_type != "text/plain":
                self._on_invalid_response(response, Error("Invalid Content-Type: {!r}.", content_type))
            else:
                raise TeamcityError(" ".join(response.text.strip().split("\n")))

        if response.stream:
            return response.iter_content(self.chunk_size), response

        if content_type != "application/json" and model is None:
            return response.content, response
        else:
            return self._parse_json_response(response, model=model)


class TeamcityEndpointConfig(Model):
    url = StringType(required=True)
    user = StringType(required=True)
    password = StringType(required=True)


def get_teamcity_client() -> TeamcityClient:
    teamcity = config.get_value("endpoints.teamcity", model=TeamcityEndpointConfig)
    return TeamcityClient(teamcity.url, teamcity.user, teamcity.password)
