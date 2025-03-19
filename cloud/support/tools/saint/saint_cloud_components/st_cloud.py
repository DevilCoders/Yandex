from helpers import *
from grpc_gw import cloud_service, cloud_service_pb2, get_grpc_channel, access_service, access_service_pb2, \
    user_account_service_pb2, user_account_service, console_cloud_service, resource_settings_pb2, folder_service, \
    folder_service_pb2, instance_service, instance_service_pb2



class St_Cloud:

    def get_cloud_by_id(self, cloud_id):
        return self.use_grpc(self.cloud_service_channel.Get,
                             cloud_service_pb2.GetCloudRequest(cloud_id=cloud_id))

    def get_resource_access_bindings(self, resource_id, resource_type):
        return self.use_grpc(self.access_binding_service_channel.ListAccessBindings,
                             access_service_pb2.ListAccessBindingsRequest(
                                 resource_id=resource_id,
                                 resource_type=resource_type,
                                 private_call=True,
                                 page_size=1000
                             ))
        # acb_channel = self.access_binding_service_channel
        # list_acb_request = access_service_pb2.ListAccessBindingsRequest(
        #     resource_id=resource_id,
        #     resource_type=resource_type,
        #     private_call=True,
        #     page_size=1000
        # )
        # acb = None
        # try:
        #     acb = acb_channel.ListAccessBindings(list_acb_request)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        #
        # return acb

    def get_users_list(self, cloud_id):
        """
        :param cloud_id: id
        :param iam_token: iam token
        :return: users grpc response
        """
        return self.use_grpc(self.cloud_service_channel.ListUsers,
                             cloud_service_pb2.ListUsersRequest(
                                 cloud_ids=[cloud_id],
                                 page_size=1000
                             ))
        # users = None
        #
        # channel = self.cloud_service_channel
        #
        # list_users_request = cloud_service_pb2.ListUsersRequest(
        #     cloud_ids=[cloud_id],
        #     page_size=1000
        # )
        # try:
        #     users = channel.ListUsers(list_users_request)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        # return users

    def get_cloud_incident_subscribers(self, cloud_id):
        """
        return only mailtechsubscriberIds of non-owners
        :param cloud_id:
        :return:
        """
        # inc_channel = self.cloud_console_service_channel
        # inc_req = resource_settings_pb2.GetResourceSettingsRequest(resource_id=cloud_id,
        #                                                            response_json_path=["/mailTechSubscriberIds", ])
        # inc = {}
        # try:
        #     inc = inc_channel.GetSettings(inc_req)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        #
        inc = self.use_grpc(self.cloud_console_service_channel.GetSettings,
                            resource_settings_pb2.GetResourceSettingsRequest(resource_id=cloud_id,
                                                                             response_json_path=[
                                                                                 "/mailTechSubscriberIds", ]))

        return json.loads(inc.json).get('mailTechSubscriberIds', [])

    def get_federated_account_email(self, user_id):
        # iam_token = self.iam_token
        # ua = ''
        # user_acc_channel = get_grpc_channel(user_account_service.UserAccountServiceStub,
        #                                     self.endpoints.iam_url[self.profile.name.upper()], iam_token)
        # ua_req = user_account_service_pb2.GetUserAccountRequest(user_account_id=user_id)
        # try:
        #     ua = user_acc_channel.Get(ua_req)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        # return ua
        return self.use_grpc(self.user_account_service_channel.Get,
                             user_account_service_pb2.GetUserAccountRequest(user_account_id=user_id))

    def get_all_users(self, cloud_id):
        """
        разбить на несколько функций, каждая из который возвращает конкретный объект
        вынести форматер в метод клауд-фасада
        передавать параметры в форматер
        :param cloud_id:
        :return:
        """

        # get user list and claims
        users = self.get_users_list(cloud_id)

        # get cloud access bindings
        acb = self.get_resource_access_bindings(cloud_id, 'resource-manager.cloud')

        # get cloud incident subscribers
        subscribers = self.get_cloud_incident_subscribers(cloud_id)

        user_roles, fa_roles = self.format_access_bindings(acb)
        final = []

        for cloud_user in users.cloud_users:
            user_id = cloud_user.subject_claims.sub
            user_login = cloud_user.subject_claims.preferred_username
            if user_id in user_roles.keys():
                user_email = self.get_federated_account_email(user_id).yandex_passport_user_account.default_email
                user_role = user_roles[user_id]
                user_passport = cloud_user.subject_claims.yandex_claims.passport_uid

            elif user_id in fa_roles.keys():
                user_email = user_login
                user_role = fa_roles[user_id]
                user_passport = 'Federated user'

            else:
                continue
            str_user_role = '\n  '.join(user_role)
            notify = user_id in subscribers or 'resource-manager.clouds.owner' in str_user_role
            result = {
                'login': user_login,
                'email': user_email,
                'passport_uid': user_passport,
                'user_id': user_id,
                'role': str_user_role,
                'is_notified': f'{Color.green if notify else ""} {notify} {Color.END if notify else ""}'

            }
            final.append(result)
        return final

    def get_owners(self, cloud_id):
        """
        :param cloud_id: cloud_id
        :return: same table rows as get_all_users
        """
        all_users = self.get_all_users(cloud_id)
        owners_list = []
        if all_users is not None:
            for user in all_users:
                if user.get('role').find('resource-manager.clouds.owner') >= 0:
                    owners_list.append(user)
        return owners_list

    def folders_get_by_cloud(self, cloud_id):

        # channel = self.folder_service_channel
        # req = folder_service_pb2.ListFoldersRequest(
        #     cloud_id=cloud_id
        # )
        # resp = None
        # try:
        #     resp = channel.List(req)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        # return resp
        return self.use_grpc(self.folder_service_channel.List,
                             folder_service_pb2.ListFoldersRequest(
                                 cloud_id=cloud_id
                             ))

    def get_instances_in_folder(self, folder_id):
        # ai_channel = self.instance_service_channel
        # ai_req = instance_service_pb2.ListInstancesRequest(folder_id=folder_id)
        # ai = None
        # try:
        #     ai = ai_channel.List(ai_req)
        # except _InactiveRpcError as e:
        #     logging.error(e.details())
        #
        # return ai
        return self.use_grpc(self.instance_service_channel.List,
                             instance_service_pb2.ListInstancesRequest(folder_id=folder_id))

    def get_all_instances(self, folders_list):
        instances_by_folder = {}
        for folder in folders_list:
            folder_id = folder.get('id')
            folder_name = folder.get('name')
            ai = self.get_instances_in_folder(folder_id)
            instances_by_folder[(folder_id, folder_name)] = []
            if ai:
                for instance in ai.instances:
                    instances_by_folder[(folder_id, folder_name)].append(self.format_instance_full(instance))

        return instances_by_folder
