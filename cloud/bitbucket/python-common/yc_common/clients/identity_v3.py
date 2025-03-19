from typing import Iterable, List

import jwt
import uuid

from yc_common.exceptions import LogicalError
from yc_common import config, misc, paging
from yc_common.clients.api import ApiClient
from yc_common.clients.models.base import (
    DEFAULT_PAGE_SIZE,
    BaseListModel,
    BasePublicModel,
    BasePublicObjectModel,
    BasePublicObjectModelV1Beta1,
)
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.constants import ServiceNames
from yc_common.models import Model, StringType, ListType, ModelType, DictType, IsoTimestampType, BooleanType


class IdentityEndpointConfig(Model):
    url = StringType(required=True)


class Cloud(BasePublicObjectModel):
    description = StringType()
    status = StringType(required=True)


class CloudList(BaseListModel):
    clouds = ListType(ModelType(Cloud), required=True)


class Folder(BasePublicObjectModel):
    cloud_id = StringType(required=True)
    status = StringType(required=True)
    description = StringType()
    labels = DictType(StringType)


class FolderList(BaseListModel):
    folders = ListType(ModelType(Folder), required=True)


class _UserAccount(BasePublicModel):
    """Deprecated."""
    id = StringType(required=True)
    email = StringType()


class YandexPassportUserAccount(BasePublicModel):
    passport_uid = StringType(required=False)
    login = StringType()
    default_email = StringType()
    is_deleted = BooleanType(default=False)


class UserAccount(BasePublicModel):
    id = StringType(required=False)
    yandexPassportUserAccount = ModelType(YandexPassportUserAccount)


class ServiceAccount(BasePublicObjectModelV1Beta1):
    pass


class ServiceAccountList(BaseListModel):
    service_accounts = ListType(ModelType(ServiceAccount), required=True)


class Role(BasePublicModel):
    id = StringType(required=True)
    description = StringType()
    permission_ids = ListType(StringType, required=True)


class RoleList(BaseListModel):
    roles = ListType(ModelType(Role), required=True)


class Subject(BasePublicModel):
    class Type:
        FEDERATED_USER = "federatedUser"
        USER_ACCOUNT = "userAccount"
        SERVICE_ACCOUNT = "serviceAccount"
        SYSTEM = "system"

        ALL = (FEDERATED_USER, USER_ACCOUNT, SERVICE_ACCOUNT, SYSTEM)

    id = StringType(required=True)
    type = StringType(required=True)  # , choices=Type.ALL)  # this constraint will return a bit later


class AccessBinding(BasePublicModel):
    role_id = StringType(required=True)
    subject = ModelType(Subject, required=True)


class AccessBindingList(BaseListModel):
    access_bindings = ListType(ModelType(AccessBinding))


class AccessBindingDelta(BasePublicModel):
    ACTION_ADD = "add"
    ACTION_REMOVE = "remove"
    ACTIONS = [ACTION_ADD, ACTION_REMOVE]

    action = StringType(required=True, choices=ACTIONS)
    access_binding = ModelType(AccessBinding, required=True)


class AccessKey(BasePublicModel):
    id = StringType(required=True)
    key_id = StringType(required=True)
    service_account_id = StringType(required=True)
    name = StringType()
    created_at = IsoTimestampType(required=True)


class AccessKeyList(BaseListModel):
    keys = ListType(ModelType(AccessKey))


class CreateAccessKeyResponse(BasePublicModel):
    key = ModelType(AccessKey, required=True)
    secret = StringType(required=True)


class TokenKey(BasePublicModel):
    id = StringType(required=True)
    service_account_id = StringType(required=False)
    user_account_id = StringType(required=False)
    created_at = IsoTimestampType(required=True)
    description = StringType(required=False)
    public_key = StringType(required=True)
    key_algorithm = StringType(required=True)


class PrivateTokenKey(TokenKey):
    private_key = StringType(required=True)


class CreateTokenKeyResponse(BasePublicModel):
    private_key = StringType(required=True)
    key = ModelType(TokenKey, required=True)


class TokenKeyList(BaseListModel):
    keys = ListType(ModelType(TokenKey))


_DEFAULT_AUD = "https://iam.api.cloud.yandex.net/iam/v1/tokens"


class JwtEncoder:
    def __init__(self, aud=_DEFAULT_AUD, expiration=3600, algorithm="PS256"):
        self.__aud = aud
        self.__expiration = expiration
        self.__algorithm = algorithm

    def encode(self, key: PrivateTokenKey):
        now = misc.timestamp()
        return jwt.encode(
            {
                "aud": self.__aud,
                "iss": key.service_account_id,
                "iat": now,
                "exp": now + self.__expiration,
            },
            key.private_key,
            algorithm=self.__algorithm,
            headers={
                "kid": key.id,
            },
        )


def _idempotent_wrapper(obj):
    if not callable(obj):
        return obj
    def wrapper(*args, **kwargs):
        if kwargs:
            extra_headers = kwargs.get("extra_headers", {})
            extra_headers.update({"X-Y-Idempotence-ID": str(uuid.uuid4())})
            kwargs["extra_headers"] = extra_headers
        return obj(*args, **kwargs)

    return wrapper


class SillyIdempotentApiClient:
    def __init__(self, *args, **kwargs):
       self.__api_client = ApiClient(*args, **kwargs)

    def __getattr__(self, item):
        if item in ApiClient.API_CLIENT_METHODS:
            return _idempotent_wrapper(getattr(self.__api_client, item))
        else:
            return getattr(self.__api_client, item)


class IdentityClient:
    SERVICE_NAME = ServiceNames.IDENTITY

    def __init__(
        self, url, oauth_token=None, timeout=10,
        jwt=None, iam_token=None, retry_temporary_errors=True,
    ):
        self.__client = SillyIdempotentApiClient(
            url + "/v1",
            timeout=timeout,
            retry_temporary_errors=retry_temporary_errors,
        )
        self.__oauth_token = oauth_token
        self.__jwt = jwt
        self.__iam_token = iam_token

        if int(oauth_token is not None) + int(jwt is not None) + int(iam_token is not None) != 1:
            raise ValueError(
                "Exactly one of `oauth_token`, `jwt` and `iam_token` is required."
            )

        if oauth_token is not None:
            self.authenticate()
        if jwt is not None:
            self.authenticate_via_jwt(jwt)
        if iam_token is not None:
            self.__client.set_iam_token(self.__iam_token)

    @classmethod
    def from_service_token_key(cls, url, key: PrivateTokenKey, encoder: JwtEncoder=None):
        if encoder is None:
            encoder = JwtEncoder()
        return cls(url, jwt=encoder.encode(key))

    def authenticate(self):
        self.__iam_token = self.get_iam_token()
        self.__client.set_iam_token(self.__iam_token)

    def authenticate_via_jwt(self, jwt):
        self.__iam_token = self.get_iam_token_via_jwt(jwt)
        self.__client.set_iam_token(self.__iam_token)

    def get_iam_token(self):
        payload = {
            "oauthToken": self.__oauth_token
        }
        return self._obtain_iam_token(payload)

    def get_iam_token_via_jwt(self, jwt):
        payload = {
            "jwt": jwt,
        }
        return self._obtain_iam_token(payload)

    @property
    def iam_token(self):
        return self.__iam_token

    def _obtain_iam_token(self, payload):
        json_credentials = self.__client.post("/tokens", payload)
        iam_token = json_credentials["iamToken"]
        return iam_token

    def list_clouds(self, page_token=None, page_size=None) -> CloudList:
        params = misc.drop_none({
            "pageToken": page_token,
            "pageSize": page_size,
        })
        return self.__client.get("/clouds", params=params, model=CloudList)

    def iter_clouds(self, page_size=DEFAULT_PAGE_SIZE) -> Iterable[Cloud]:
        return paging.iter_items(self.list_clouds, "clouds", page_size)

    def get_cloud(self, cloud_id) -> Cloud:
        return self.__client.get("/clouds/" + cloud_id, model=Cloud)

    def list_folders(self, cloud_id, page_token=None, page_size=None) -> FolderList:
        params = misc.drop_none({
            "cloudId": cloud_id,
            "pageToken": page_token,
            "pageSize": page_size,
        })
        return self.__client.get("/folders", params=params, model=FolderList)

    def iter_folders(self, cloud_id, page_size=DEFAULT_PAGE_SIZE) -> Iterable[Folder]:
        return paging.iter_items(self.list_folders, "folders", page_size, cloud_id)

    def get_folder(self, folder_id) -> Folder:
        path = "/folders/" + folder_id
        return self.__client.get(path, model=Folder)

    def create_folder(self, cloud_id, name, description=None, labels=None) -> OperationV1Beta1:
        request = misc.drop_none({
            "cloudId": cloud_id,
            "name": name,
            "description": description,
            "labels": labels,
        })
        return self.__client.post("/folders", request=request, model=OperationV1Beta1)

    def update_folder(self, folder_id, name, description, labels) -> OperationV1Beta1:
        request = {"folderId": folder_id, "name": name, "description": description, "labels": labels}
        return self.__client.patch("/folders", request, model=OperationV1Beta1)

    def delete_folder(self, folder_id) -> OperationV1Beta1:
        path = "/folders/" + folder_id
        return self.__client.delete(path, model=OperationV1Beta1)

    def get_user_account_by_id(self, user_account_id):
        path = "/userAccounts/" + user_account_id
        return self.__client.get(path, model=UserAccount)

    def get_user_account_by_email(self, user_account_email):
        """Deprecated, use `get_passport_account_by_login` instead."""
        params = {"email": user_account_email}
        return self.__client.get("/userAccounts:byEmail", params=params, model=_UserAccount)

    def get_passport_account_by_login(self, login, allow_unregistered_users=None):
        params = {"login": login, "allowUnregisteredUsers": allow_unregistered_users}
        return self.__client.get("/yandexPassportUserAccounts:byLogin", params=params, model=UserAccount)

    def get_passport_account_by_passport_uid(self, passport_uid):
        params = {"passportUid": passport_uid}
        return self.__client.get("/yandexPassportUserAccounts:byPassportUid", params=params, model=UserAccount)

    def create_service_account(self, folder_id, name, description=None) -> OperationV1Beta1:
        request = misc.drop_none({
            "folderId": folder_id,
            "name": name,
            "description": description,
        })
        return self.__client.post("/serviceAccounts", request, model=OperationV1Beta1)

    def update_service_account(self, service_account_id, name=None, description=None, labels=None) -> OperationV1Beta1:
        request = misc.drop_none({
            "serviceAccountId": service_account_id,
            "name": name,
            "description": description,
            "labels": labels,
        })
        return self.__client.patch("/serviceAccounts", request, model=OperationV1Beta1)

    def list_service_accounts(self, folder_id, page_token=None, page_size=None) -> ServiceAccountList:
        params = misc.drop_none({
            "folderId": folder_id,
            "pageToken": page_token,
            "pageSize": page_size,
        })
        return self.__client.get("/serviceAccounts", params=params, model=ServiceAccountList)

    def iter_service_accounts(self, folder_id, page_size=DEFAULT_PAGE_SIZE) -> Iterable[ServiceAccount]:
        return paging.iter_items(self.list_service_accounts, "service_accounts", page_size, folder_id)

    def get_service_account(self, service_account_id) -> ServiceAccount:
        path = "/serviceAccounts/" + service_account_id
        return self.__client.get(path, model=ServiceAccount)

    def delete_service_account(self, service_account_id) -> OperationV1Beta1:
        path = "/serviceAccounts/" + service_account_id
        return self.__client.delete(path, model=OperationV1Beta1)

    def issue_service_account_token(self, service_account_id, instance_id):
        request = misc.drop_none({
            "instanceId": instance_id,
        })
        res = self.__client.post("/serviceAccounts/{}:issueToken".format(service_account_id), request)
        return res["iamToken"]

    def list_roles(self, page_token=None, page_size=None) -> RoleList:
        params = {
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get("/roles", params=params, model=RoleList)

    def iter_roles(self, page_size=DEFAULT_PAGE_SIZE) -> Iterable[Role]:
        return paging.iter_items(self.list_roles, "roles", page_size)

    def list_cloud_access_bindings(self, cloud_id, show_system_bindings=False, page_token=None, page_size=None):
        params = {
            "pageToken": page_token,
            "pageSize": page_size,
            "showSystemBindings": show_system_bindings,
        }
        return self.__client.get(
            "/clouds/{}:listAccessBindings".format(cloud_id), params=params, model=AccessBindingList
        )

    def iter_cloud_access_bindings(self, cloud_id, show_system_bindings=False, page_size=DEFAULT_PAGE_SIZE):
        return paging.iter_items(
            self.list_cloud_access_bindings, "access_bindings", page_size,
            cloud_id, show_system_bindings=show_system_bindings,
        )

    def list_folder_access_bindings(self, folder_id, show_system_bindings=False, page_token=None, page_size=None):
        params = {
            "pageToken": page_token,
            "pageSize": page_size,
            "showSystemBindings": show_system_bindings,
        }
        return self.__client.get(
            "/folders/{}:listAccessBindings".format(folder_id), params=params, model=AccessBindingList
        )

    def iter_folder_access_bindings(self, folder_id, show_system_bindings=False, page_size=DEFAULT_PAGE_SIZE):
        return paging.iter_items(
            self.list_folder_access_bindings, "access_bindings", page_size,
            folder_id, show_system_bindings=show_system_bindings,
        )

    def list_service_account_access_bindings(self, service_account_id, show_system_bindings=False, page_token=None, page_size=None):
        params = {
            "pageToken": page_token,
            "pageSize": page_size,
            "showSystemBindings": show_system_bindings,
        }
        return self.__client.get(
            "/serviceAccounts/{}:listAccessBindings".format(service_account_id), params=params, model=AccessBindingList
        )

    def iter_service_account_access_bindings(self, service_account_id, show_system_bindings=False, page_size=DEFAULT_PAGE_SIZE):
        return paging.iter_items(
            self.list_service_account_access_bindings, "access_bindings", page_size,
            service_account_id, show_system_bindings=show_system_bindings,
        )

    def update_cloud_access_bindings(self, cloud_id, access_binding_deltas, allow_system_roles=False):
        request = {
            "accessBindingDeltas": access_binding_deltas,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/clouds/{}:updateAccessBindings".format(cloud_id), request, model=OperationV1Beta1)

    def update_folder_access_bindings(self, folder_id, access_binding_deltas, allow_system_roles=False):
        request = {
            "accessBindingDeltas": access_binding_deltas,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/folders/{}:updateAccessBindings".format(folder_id), request, model=OperationV1Beta1)

    def update_service_account_access_bindings(self, service_account_id, access_binding_deltas, allow_system_roles=False):
        request = {
            "accessBindingDeltas": access_binding_deltas,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/serviceAccounts/{}:updateAccessBindings".format(service_account_id), request, model=OperationV1Beta1)

    def set_cloud_access_bindings(self, cloud_id, access_bindings: List[AccessBinding], allow_system_roles=False):
        request = {
            "accessBindings": access_bindings,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/clouds/{}:setAccessBindings".format(cloud_id), request, model=OperationV1Beta1)

    def set_folder_access_bindings(self, folder_id, access_bindings: List[AccessBinding], allow_system_roles=False):
        request = {
            "accessBindings": access_bindings,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/folders/{}:setAccessBindings".format(folder_id), request, model=OperationV1Beta1)

    def set_service_account_access_bindings(self, service_account_id, access_bindings: List[AccessBinding], allow_system_roles=False):
        request = {
            "accessBindings": access_bindings,
            "allowSystemRoles": allow_system_roles,
        }
        return self.__client.post("/serviceAccounts/{}:setAccessBindings".format(service_account_id), request, model=OperationV1Beta1)

    def list_access_keys(self, service_account_id) -> AccessKeyList:
        return self.__client.get("/serviceAccounts/{}/accessKeys".format(service_account_id), model=AccessKeyList)

    def create_access_key(self, service_account_id, name=None) -> CreateAccessKeyResponse:
        request = misc.drop_none({
            "name": name,
        })
        return self.__client.post("/serviceAccounts/{}/accessKeys".format(service_account_id), request, model=CreateAccessKeyResponse)

    def delete_access_key(self, service_account_id, key_name):
        return self.__client.delete("/serviceAccounts/{}/accessKeys/{}".format(service_account_id, key_name))

    def create_service_account_token_key(self, service_account_id=None, description=None, format=None, key_algorithm=None) -> CreateTokenKeyResponse:
        request = misc.drop_none({
            "serviceAccountId": service_account_id,
            "description": description,
            "format": format,
            "keyAlgorithm": key_algorithm,
        })
        return self.__client.post("/keys", request=request, model=CreateTokenKeyResponse)

    def get_token_key(self, key_id, format=None) -> TokenKey:
        request = misc.drop_none({
            "format": format,
        })
        return self.__client.get("/keys/{}".format(key_id), request=request, model=TokenKey)

    def list_token_keys(self, service_account_id=None, format=None, page_token=None, page_size=None) -> TokenKeyList:
        request = misc.drop_none({
            "serviceAccountId": service_account_id,
            "pageToken": page_token,
            "pageSize": page_size,
            "format": format,
        })
        return self.__client.get("/keys", request=request, model=TokenKeyList)

    def delete_key(self, key_id) -> OperationV1Beta1:
        return self.__client.delete("/keys/{}".format(key_id), model=OperationV1Beta1)


def get_identity_client(oauth_token, retry_temporary_errors=True) -> IdentityClient:
    return IdentityClient(
        config.get_value("endpoints.identity", model=IdentityEndpointConfig).url,
        oauth_token,
        retry_temporary_errors=retry_temporary_errors,
    )
