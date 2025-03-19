import itertools

from schematics.types import ListType, ModelType
from typing import Iterable, List

from yc_common import config
from yc_common import exceptions
from yc_common import misc
from yc_common import models
from yc_common import paging
from yc_common.clients.identity_v3 import Cloud, CloudList, Folder, AccessBinding, AccessBindingList
from yc_common.clients.models.base import DEFAULT_PAGE_SIZE, BaseListModel, BasePublicModel
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.clients.api import ApiClient
from yc_common.misc import drop_none


class CloudStatuses:
    CREATING = "CREATING"
    ACTIVE = "ACTIVE"
    DELETING = "DELETING"
    BLOCKED = "BLOCKED"


class FolderStatuses:
    ACTIVE = "ACTIVE"
    DELETING = "DELETING"


class CloudBlockReasons:
    BILLING = "billing"


class ListCloudsResponse(BaseListModel):
    result = ListType(ModelType(Cloud))


class ListFoldersResponse(BaseListModel):
    result = ListType(ModelType(Folder))


class User(models.Model):
    id = models.StringType(required=True)
    login = models.StringType()
    firstName = models.StringType()
    lastName = models.StringType()
    avatar = models.StringType()


class UserList(BaseListModel):
    users = ListType(ModelType(User), required=True)


class PassportUserAccount(BasePublicModel):
    id = models.StringType()
    login = models.StringType()
    passport_uid = models.StringType()
    email = models.StringType()
    first_name = models.StringType()
    last_name = models.StringType()
    phone = models.StringType()
    timezone = models.StringType()


class CloudCreatorInfo(BasePublicModel):
    cloud = ModelType(Cloud, required=True)
    passport_user_account = ModelType(PassportUserAccount, required=True)


class ListCloudCreatorsResponse(BaseListModel):
    cloud_creators = ListType(ModelType(CloudCreatorInfo), required=True)


class CloudDefaultZone(BasePublicModel):
    default_zone = models.StringType()
    effective_zone = models.StringType()
    zone_overridden = models.BooleanType()


class IdentityPrivateClient:
    """
    Only get/set status for clouds and folders are supported
    """
    def __init__(self, url, retry_temporary_errors=True, timeout=10, iam_token=None):
        self.__client = ApiClient(
            url + "/v1",
            timeout=timeout,
            iam_token=iam_token,
            retry_temporary_errors=retry_temporary_errors,
        )

    def ping(self):
        return self.__client.get("/ping")

    def clear_sync_cache(self):
        return self.__client.post("/sync:clearCaches", request={})

    def list_clouds(self, cloud_name=None, cloud_id=None, page_token=None, page_size=None) -> ListCloudsResponse:
        params = misc.drop_none({
            "id": cloud_id,
            "name": cloud_name,
            "pageSize": page_size,
            "pageToken": page_token,
        })

        return self.__client.get("/allClouds", params=params, model=ListCloudsResponse)

    def iter_clouds(self, cloud_name=None, cloud_id=None, page_size=DEFAULT_PAGE_SIZE) -> Iterable[Cloud]:
        return paging.iter_items(
            self.list_clouds, entity_name="result", page_size=page_size,
            cloud_name=cloud_name, cloud_id=cloud_id,
        )

    def list_folders(self, folder_id=None, folder_name=None, cloud_id=None,
                     page_token=None, page_size=None) -> ListFoldersResponse:
        params = misc.drop_none({
            "id": folder_id,
            "name": folder_name,
            "cloudId": cloud_id,
            "pageSize": page_size,
            "pageToken": page_token,
        })

        return self.__client.get("/allFolders", params=params, model=ListFoldersResponse)

    def iter_folders(self, folder_id=None, folder_name=None, cloud_id=None,
                     page_size=DEFAULT_PAGE_SIZE) -> Iterable[Folder]:
        return paging.iter_items(
            self.list_folders, entity_name="result", page_size=page_size,
            folder_id=folder_id, folder_name=folder_name, cloud_id=cloud_id,
        )

    def list_cloud_creators(self, page_token=None, page_size=None) -> ListCloudCreatorsResponse:
        params = misc.drop_none({
            "pageSize": page_size,
            "pageToken": page_token,
        })
        return self.__client.get("/clouds:creators", params=params, model=ListCloudCreatorsResponse)

    def iter_cloud_creators(self, page_token=None, page_size=None) -> Iterable[CloudCreatorInfo]:
        return paging.iter_items(self.list_cloud_creators, entity_name="cloud_creators", page_size=page_size)

    def list_users(self, cloud_id, page_token=None, page_size=None) -> UserList:
        return self.__client.get("/cloud/" + cloud_id + "/users", model=UserList)

    def iter_users(self, page_size=DEFAULT_PAGE_SIZE) -> Iterable[User]:
        return paging.iter_items(self.list_users, "users", page_size)

    def get_passport_uid(self, user_id: str):
        class Result(models.Model):
            passportUid = models.StringType(required=True)

        result = self.__client.get("/passportUid?subjectId={}".format(user_id), model=Result)
        return result.passportUid

    def create_cloud(self, name, oauth_token=None, owner_login=None, description=None):
        request = drop_none({
            "name": name,
            "description": description,
            "ownerLogin": owner_login,
        })
        if oauth_token is not None and owner_login is not None:
            raise exceptions.LogicalError("Only one of `oauth_token` and `owner_login` should be passed.")
        elif oauth_token is None and owner_login is None:
            raise exceptions.LogicalError("Only one of `oauth_token` and `owner_login` should be passed.")
        elif oauth_token is not None:
            headers = {"Authorization": "OAuth {}".format(oauth_token)}
        elif owner_login is not None:
            request["ownerLogin"] = owner_login
            headers = dict()
        return self.__client.post("/cloud", request, extra_headers=headers, model=Cloud)

    def create_cloud_by_creator_uid(self, creator_uid, monetary_offer_id=None, description=None):
        class Result(models.Model):
            id = models.StringType(required=True)

        request = drop_none({
            "creatorUid": creator_uid,
            "description": description,
            "monetaryOfferId": monetary_offer_id,
        })
        return self.__client.post("/clouds:byCreatorUid", request, model=Result)

    def set_cloud_policy(self, cloud_id, policy_id):
        self.__client.post("/cloud/" + cloud_id + "/policies", {
            "policy_id": policy_id,
        })

    def get_cloud_status(self, cloud_id: str) -> str:
        class Result(models.Model):
            status = models.StringType(required=True)

        return self.__client.get("/clouds/{}/status".format(cloud_id),
                                 model=Result,
                                 ).status

    def set_cloud_status(self, cloud_id: str, status: str, old_status: str=None):
        data = misc.drop_none({
            "old_status": old_status,
            "status": status,
        })
        return self.__client.post(
            "/clouds/{}/status".format(cloud_id),
            data,
        )

    def list_cloud_permission_stages(self, cloud_id: str) -> Iterable[str]:
        class ListPermissionStagesResponse(BaseListModel):
            permission_stages = ListType(models.StringType, required=True)

        return self.__client.get(
            "/clouds/{}:getPermissionStages".format(cloud_id),
            model=ListPermissionStagesResponse
        ).permission_stages

    def add_cloud_permission_stages(self, cloud_id: str, permission_stages: Iterable[str]):
        data = {"permissionStages": permission_stages}
        self.__client.post("/clouds/{}:addPermissionStages".format(cloud_id), data)

    def remove_cloud_permission_stages(self, cloud_id: str, permission_stages: Iterable[str]):
        data = {"permissionStages": permission_stages}
        self.__client.post("/clouds/{}:removePermissionStages".format(cloud_id), data)

    def get_folder_status(self, folder_id: str) -> str:
        class Result(models.Model):
            status = models.StringType(required=True)

        return self.__client.get("/folders/{}/status".format(folder_id),
                                 model=Result,
                                 ).status

    def set_folder_status(self, folder_id: str, status: str, old_status: str=None):
        data = misc.drop_none({
            "old_status": old_status,
            "status": status,
        })
        return self.__client.post(
            "/folders/{}/status".format(folder_id),
            data,
        )

    def list_root_access_bindings(self, subject_id=None, role_id=None, page_token=None, page_size=None):
        params = misc.drop_none({
            "subjectId": subject_id,
            "roleId": role_id,
            "pageToken": page_token,
            "pageSize": page_size,
        })
        return self.__client.get("/rootBindings:list", params=params, model=AccessBindingList)

    def iter_root_access_bindings(self, page_size=DEFAULT_PAGE_SIZE):
        return paging.iter_items(self.list_root_access_bindings, "access_bindings", page_size)

    def update_root_access_bindings(self, access_binding_deltas):
        request = {
            "accessBindingDeltas": access_binding_deltas
        }
        return self.__client.post("/rootBindings:update", request, model=OperationV1Beta1)

    def iter_clouds_by_login(self, login, page_size=DEFAULT_PAGE_SIZE) -> Iterable[Cloud]:
        return paging.iter_items(self.list_clouds_by_login, "clouds", page_size, login)

    def list_clouds_by_login(self, login, page_token=None, page_size=None) -> CloudList:
        params = misc.drop_none({
            "login": login,
            "pageToken": page_token,
            "pageSize": page_size,
        })
        return self.__client.get("/clouds:byLogin", params=params, model=CloudList)

    def get_cloud_creator(self, cloud_id):
        url = "/clouds/{}:getCreator".format(cloud_id)
        return self.__client.get(url, model=PassportUserAccount)

    def block_clouds(self, cloud_ids, reason):
        request = {
            "cloudIds": cloud_ids,
            "reason": reason,
        }
        return self.__client.post("/blockClouds", request)

    def unblock_clouds(self, cloud_ids, reason):
        request = {
            "cloudIds": cloud_ids,
            "reason": reason,
        }
        return self.__client.post("/unblockClouds", request)

    def list_user_passport_info(self, user_ids):
        class _DomainPassportInfo(models.Model):
            domain = models.StringType(required=False)
            domid = models.StringType(required=False)
            hosted = models.BooleanType(required=False)

        class _UserPassportInfo(models.Model):
            id = models.StringType(required=True)
            passportUid = models.StringType(required=True)
            firstName = models.StringType(required=False)
            lastName = models.StringType(required=False)
            login = models.StringType(required=False)
            email = models.StringType(required=False)
            avatar = models.StringType(required=False)
            phones = ListType(models.StringType)
            defaultPhone = models.StringType(required=False)
            karma = models.IntType(required=False)
            domain = ModelType(_DomainPassportInfo, required=False)

        class Result(models.Model):
            userAccounts = ListType(ModelType(_UserPassportInfo))

        if isinstance(user_ids, str):
            joined_ids = user_ids
        else:
            joined_ids = ",".join(user_ids)
        return self.__client.get(
            "/userAccounts:passportInfo",
            params={"id": joined_ids},
            model=Result,
        )

    def iter_user_passport_info(self, user_ids, page_size=DEFAULT_PAGE_SIZE):
        if isinstance(user_ids, str):
            user_ids = [user_ids]
        user_id_iter = iter(user_ids)

        while True:
            user_id_slice = list(itertools.islice(user_id_iter, page_size))
            if user_id_slice == []:
                break
            for item in self.list_user_passport_info(user_id_slice).userAccounts:
                yield item

    def create_folder_with_predefined_id(
        self, cloud_id, folder_id, name, description=None, labels=None,
    ) -> OperationV1Beta1:
        request = misc.drop_none({
            "id": folder_id,
            "cloudId": cloud_id,
            "name": name,
            "description": description,
            "labels": labels,
        })
        return self.__client.post(
            "/folders:withId",
            request=request,
            model=OperationV1Beta1,
        )

    def create_service_account_with_predefined_id(
            self, folder_id, service_account_id, name, description=None, labels=None,
    ) -> OperationV1Beta1:
        request = misc.drop_none({
            "id": service_account_id,
            "folderId": folder_id,
            "name": name,
            "description": description,
            "labels": labels,
        })
        return self.__client.post(
            "/serviceAccounts:withId",
            request=request,
            model=OperationV1Beta1,
        )

    def list_resource_access_bindings(
        self, resource_type, resource_id, show_system_bindings=False,
        page_token=None, page_size=None,
    ):
        params = {
            "resourceType": resource_type,
            "resourceId": resource_id,
            "showSystemBindings": show_system_bindings,
            "pageToken": page_token,
            "pageSize": page_size,
        }
        return self.__client.get(
            "/listAccessBindings", params=params, model=AccessBindingList,
        )

    def iter_resource_access_bindings(
        self, resource_type, resource_id, show_system_bindings=False,
        page_size=DEFAULT_PAGE_SIZE,
    ):
        return paging.iter_items(
            self.list_resource_access_bindings, "access_bindings", page_size,
            resource_type, resource_id, show_system_bindings=show_system_bindings,
        )

    def update_resource_access_bindings(self, resource_type, resource_id, access_binding_deltas):
        request = {
            "resourceType": resource_type,
            "resourceId": resource_id,
            "accessBindingDeltas": access_binding_deltas,
        }
        return self.__client.post("/updateAccessBindings", request)

    def set_resource_access_bindings(self, resource_type, resource_id, access_bindings: List[AccessBinding]):
        request = {
            "resourceType": resource_type,
            "resourceId": resource_id,
            "accessBindings": access_bindings,
        }
        return self.__client.post("/setAccessBindings", request)

    def get_cloud_default_zone(self, cloud_id) -> CloudDefaultZone:
        url = "/clouds/{}:defaultZone".format(cloud_id)
        return self.__client.get(url, model=CloudDefaultZone)


class IdentityPrivateEndpointConfig(models.Model):
    url = models.StringType(required=True)


def get_identity_private_client() -> IdentityPrivateClient:
    return IdentityPrivateClient(
        config.get_value("endpoints.identity_private", model=IdentityPrivateEndpointConfig).url,
        retry_temporary_errors=True,
    )
