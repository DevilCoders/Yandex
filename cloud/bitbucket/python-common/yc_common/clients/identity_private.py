from schematics.types import ListType, ModelType

from yc_common.clients.identity_v2 import Organization, Project
from yc_common.clients.api import ApiClient
from yc_common.misc import drop_none
from yc_common.models import Model, StringType
from yc_common import config
from yc_common import exceptions


class OrganizationStatus:
    NEW = 'new'
    BLOCKED = 'blocked'
    OK = 'ok'


class ProjectStatus:
    NEW = 'new'
    SYNCED = 'synced'
    OK = 'ok'


class IdentityPrivateClient:
    """
    Only get/set status for organizations and projects are supported
    """

    def __init__(self, url, retry_temporary_errors=None, timeout=10):
        self.__client = ApiClient(url + "/v1", timeout=timeout)
        self.retry_temporary_errors = retry_temporary_errors

    def ping(self):
        return self.__client.get("/ping")

    def list_organizations(self):
        class Result(Model):
            result = ListType(ModelType(Organization), required=True)

        return self.__client.get("/org", model=Result)

    def list_projects(self):
        class Result(Model):
            result = ListType(ModelType(Project), required=True)

        return self.__client.get("/allProjects", model=Result)

    def get_passport_uid(self, user_id: str):
        class Result(Model):
            uid = StringType(required=True)

        result = self.__client.get("/user/{}/passport".format(user_id), model=Result)
        return result.uid

    def create_organization(self, name, oauth_token=None, owner_login=None, description=None):
        class Result(Model):
            result = StringType(required=True)
            id = StringType(required=True)

        request = drop_none({
            "name": name,
            "description": description,
            "owner_login": owner_login,
        })
        if oauth_token is not None and owner_login is not None:
            raise exceptions.LogicalError("Only one of `oauth_token` and `owner_login` should be passed.")
        elif oauth_token is None and owner_login is None:
            raise exceptions.LogicalError("Only one of `oauth_token` and `owner_login` should be passed.")
        elif oauth_token is not None:
            headers = {"Authorization": "OAuth {}".format(oauth_token)}
        elif owner_login is not None:
            request["owner_login"] = owner_login
            headers = dict()
        return self.__client.post("/org", request, extra_headers=headers, model=Result)

    def get_organization_status(self, organization_id: str) -> str:
        class Result(Model):
            status = StringType(required=True)

        return self.__client.get("/org/{}/status".format(organization_id),
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors).status

    def set_organization_status(self, organization_id: str, status: str):
        data = {'status': status}
        return self.__client.post("/org/{}/status".format(organization_id), data,
                                  retry_temporary_errors=self.retry_temporary_errors)

    def create_organization_policy_assignments(self, organization_id: str, policy_id: str):
        data = {"policy_id": policy_id}
        return self.__client.post("/org/{}/policies".format(organization_id), data,
                                  retry_temporary_errors=self.retry_temporary_errors)

    def get_project_status(self, organization_id: str, project_id: str) -> str:
        class Result(Model):
            status = StringType(required=True)

        return self.__client.get("/org/{}/project/{}/status".format(organization_id, project_id),
                                 model=Result,
                                 retry_temporary_errors=self.retry_temporary_errors
                                 ).status

    def set_project_status(self, organization_id: str, project_id: str, status: str):
        data = {'status': status}
        return self.__client.post("/org/{}/project/{}/status".format(organization_id, project_id),
                                  data,
                                  retry_temporary_errors=self.retry_temporary_errors)

    def delete_project(self, project_id):
        path = "/projects/" + project_id
        return self.__client.delete(path)


class IdentityPrivateEndpointConfig(Model):
    url = StringType(required=True)


def get_identity_private_client() -> IdentityPrivateClient:
    return IdentityPrivateClient(
        config.get_value("endpoints.identity_private", model=IdentityPrivateEndpointConfig).url)
