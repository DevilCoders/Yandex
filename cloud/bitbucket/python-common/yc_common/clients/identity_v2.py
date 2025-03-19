from typing import Iterable

from yc_common import paging
from yc_common.clients.api import ApiClient
from yc_common.clients.models import base as base_models
from yc_common.clients.models import operations as operations_models
from yc_common.constants import ServiceNames
from yc_common.misc import not_implemented
from yc_common.models import Model, BooleanType, IntType, StringType, ListType, ModelType
from yc_requests.credentials import YandexCloudCredentials


class IdentityEndpointConfig(Model):
    url = StringType(required=True)


class ResourceTypes:
    ORGANIZATION = "organization"
    PROJECT = "project"


class SubjectTypes:
    PASSPORT = "passport"
    IAM = "iam"


class Organization(Model):
    id = StringType(required=True)
    name = StringType(required=True)
    status = StringType(required=True)
    requestSignatureEnabled = BooleanType(required=False)


class OrganizationList(base_models.BaseListModel):
    organizations = ListType(ModelType(Organization), required=True)


class Project(Model):
    organizationId = StringType(required=True)
    id = StringType(required=True)
    status = StringType(required=True)
    name = StringType(required=True)


class ProjectList(base_models.BaseListModel):
    projects = ListType(ModelType(Project), required=True)


class User(Model):
    id = StringType(required=True)
    login = StringType(required=True)
    organizationId = StringType(required=True)
    avatar = StringType(required=True)
    firstName = StringType(required=True)
    lastName = StringType(required=True)
    projects = IntType(required=True)


class UserList(base_models.BaseListModel):
    class User(Model):
        id = StringType(required=True)
        login = StringType(required=True)
        organizationId = StringType(required=True)
        projects = IntType(required=True)

    users = ListType(ModelType(User), required=True)


class Role(Model):
    slug = StringType(required=True)
    organization = StringType(required=True)


class RoleList(base_models.BaseListModel):
    roles = ListType(ModelType(Role), required=True)


class Assignment(Model):
    id = StringType(required=True)
    organizationId = StringType(required=True)
    resourceId = StringType(required=True)
    resourceType = StringType(required=True)
    subjectId = StringType(required=True)
    subjectType = StringType(required=True)
    roleSlug = StringType(required=True)


class AssignmentList(base_models.BaseListModel):
    assignments = ListType(ModelType(Assignment))


class AccessKey(Model):
    keyId = StringType(required=True)
    secretKey = StringType()


class IdentityClient:
    SERVICE_NAME = ServiceNames.IDENTITY

    def __init__(self, url, oauth_token, org_id=None, timeout=10):
        self.__client = ApiClient(url + "/v1alpha1", timeout=timeout)
        self.__oauth_token = oauth_token
        self.__org_id_value = None
        if org_id is not None:
            self.authenticate_in_organization(org_id)

    @property
    def __org_id(self):
        if self.__org_id_value is None:
            raise ValueError(
                "Organization ID is not set. "
                "Please authenticate the client using the authenticate_in_organization method."
            )
        return self.__org_id_value

    def authenticate_in_organization(self, org_id, force_reissue=False):
        """Authenticates the client in an organization.

        This method should always be called before any other operations,
        except creating or listing organizations."""

        if force_reissue or org_id != self.__org_id_value:
            payload = {
                "organizationId": org_id,
            }
            extra_headers = {"Authorization": "OAuth {}".format(self.__oauth_token)}
            json_credentials = self.__client.post("/auth/oauth", payload, extra_headers=extra_headers)
            self.__client.set_iam_token(json_credentials['token'])
            self.__org_id_value = org_id

    def get_credentials(self) -> YandexCloudCredentials:
        json_credentials = self.__client.post("/auth/token", {})
        return YandexCloudCredentials(json_credentials['token'], json_credentials['secret_key'])

    def list_organizations(self, page_token=None, page_size=None) -> OrganizationList:
        if self.__oauth_token is not None:
            extra_headers = {"Authorization": "OAuth {}".format(self.__oauth_token)}
        else:
            extra_headers = None
        params = {
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/organizations", params=params, model=OrganizationList, extra_headers=extra_headers)

    def iter_organizations(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[Organization]:
        return paging.iter_items(self.list_organizations, "organizations", page_size)

    def get_organization(self) -> Organization:
        return self.__client.get("/organizations/" + self.__org_id, model=Organization)

    def list_users(self, page_token=None, page_size=None) -> UserList:
        params = {
            "organizationId": self.__org_id,
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/users", params=params, model=UserList)

    def iter_users(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[User]:
        return paging.iter_items(self.list_users, "users", page_size)

    def get_user(self, user_id) -> User:
        path = "/users/" + user_id
        return self.__client.get(path, model=User)

    def create_user(self, login, user_type=SubjectTypes.PASSPORT) -> operations_models.OperationV1Alpha1:
        request = {
            "organizationId": self.__org_id,
            "login": login,
            "userType": user_type,
        }
        return self.__client.post("/users", request, model=operations_models.OperationV1Alpha1)

    def delete_user(self, user_id) -> operations_models.OperationV1Alpha1:
        path = "/users/" + user_id
        return self.__client.delete(path, model=operations_models.OperationV1Alpha1)

    def list_projects(self, page_token=None, page_size=None) -> ProjectList:
        params = {
            "organizationId": self.__org_id,
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/projects", params=params, model=ProjectList)

    def iter_projects(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[Project]:
        return paging.iter_items(self.list_projects, "projects", page_size)

    def get_project(self, project_id) -> Project:
        path = "/projects/" + project_id
        return self.__client.get(path, model=Project)

    def create_project(self, name) -> operations_models.OperationV1Alpha1:
        request = {
            "organizationId": self.__org_id,
            "name": name,
        }
        return self.__client.post("/projects", request=request, model=operations_models.OperationV1Alpha1)

    def update_project(self, project_id, name) -> operations_models.OperationV1Alpha1:
        path = "/projects/" + project_id
        request = {"name": name}
        return self.__client.post(path, request, model=operations_models.OperationV1Alpha1)

    def delete_project(self, project_id) -> operations_models.OperationV1Alpha1:
        path = "/projects/" + project_id
        return self.__client.delete(path, model=operations_models.OperationV1Alpha1)

    def list_roles(self, page_token=None, page_size=None) -> RoleList:
        params = {
            "organizationId": self.__org_id,
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/roles", params=params, model=RoleList)

    def iter_roles(self, page_size=base_models.DEFAULT_PAGE_SIZE) -> Iterable[Role]:
        return paging.iter_items(self.list_roles, "roles", page_size)

    def list_assignments(
            self,
            resource_type=None,
            resource_id=None,
            subject_type=None,
            subject_id=None,
            role_slug=None,
            page_token=None,
            page_size=None,
    ) -> AssignmentList:
        params = {
            "organizationId": self.__org_id,
            "resourceType": resource_type,
            "resourceId": resource_id,
            "subjectType": subject_type,
            "subjectId": subject_id,
            "roleSlug": role_slug,
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/roleAssignments", params=params, model=AssignmentList)

    def iter_assignments(
            self,
            resource_type=None,
            resource_id=None,
            subject_type=None,
            subject_id=None,
            role_slug=None,
            page_size=base_models.DEFAULT_PAGE_SIZE,
    ) -> Iterable[Assignment]:
        return paging.iter_items(
            self.list_assignments, "assignments", resource_type=resource_type, resource_id=resource_id,
            subject_type=subject_type, subject_id=subject_id, role_slug=role_slug, page_size=page_size,
        )

    def create_assignment(self, resource_type, resource_id, subject_id, role_slug) -> operations_models.OperationV1Alpha1:
        request = {
            "organizationId": self.__org_id,
            "resourceType": resource_type,
            "resourceId": resource_id,
            "subjectId": subject_id,
            "roleSlug": role_slug,
        }
        return self.__client.post("/roleAssignments", request, model=operations_models.OperationV1Alpha1)

    def delete_assignment(self, assignment_id) -> operations_models.OperationV1Alpha1:
        path = "/roleAssignments/" + assignment_id
        return self.__client.delete(path)

    def list_access_keys(self, user_id) -> AccessKey:
        class Result(Model):
            keys = ModelType(AccessKey, required=True)
        return self.__client.get("/users/{}/accessKeys".format(user_id), model=Result).result

    def create_access_key(self, user_id) -> AccessKey:
        return self.__client.post("/users/{}/accessKeys".format(user_id), {}, model=AccessKey).result

    def delete_access_key(self, user_id, key_id):
        return self.__client.delete("/users/{}/accessKeys/{}".format(user_id, key_id))
