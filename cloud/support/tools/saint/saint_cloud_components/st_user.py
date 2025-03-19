from grpc_gw import *


class St_User:



    def get_user_roles(self, u_id):

        # ab_channel = self.backoffice_access_bindings_service_channel
        # ab_req = ab_service_pb2.ListSubjectAccessBindingsRequest(subject_id=u_id, page_size=1000)
        # roles = None
        # try:
        #     roles = ab_channel.ListBySubject(ab_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return roles.access_bindings
        return self.use_grpc(self.backoffice_access_bindings_service_channel.ListBySubject,
                             ab_service_pb2.ListSubjectAccessBindingsRequest(subject_id=u_id, page_size=1000))

    def get_user_account(self, user_id):
        # user_acc_channel = self.user_account_service_channel
        # ua_req = user_account_service_pb2.GetUserAccountRequest(user_account_id=user_id)
        # user = None
        # try:
        #     user = user_acc_channel.Get(ua_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return user
        return self.use_grpc(self.user_account_service_channel.Get,
                             user_account_service_pb2.GetUserAccountRequest(user_account_id=user_id))

    def get_yandex_passport_user_account(self, user_id):
        # user_acc_pub_channel = self.yandex_passport_user_account_service_channel
        # ua_pub_req = user_account_pub_service_pb2.GetUserAccountByLoginRequest(login=user_id)
        # user = None
        # try:
        #     user = user_acc_pub_channel.GetByLogin(ua_pub_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return user
        return self.use_grpc(self.yandex_passport_user_account_service_channel.GetByLogin,
                             user_account_pub_service_pb2.GetUserAccountByLoginRequest(login=user_id))

    def get_service_account(self, subject_id):
        # service_acc_channel = self.service_account_channel
        # sa_req = service_account_service_pb2.GetServiceAccountRequest(service_account_id=subject_id)
        # sa = None
        # try:
        #     sa = service_acc_channel.Get(sa_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return sa
        return self.use_grpc(self.service_account_channel.Get,
                             service_account_service_pb2.GetServiceAccountRequest(service_account_id=subject_id))

    def account_get_from_organization(self, account_id: str, organization_id='yc.organization-manager.yandex'):
        if account_id.find('@') > 0:
            filter_type = "claims.email"
        else:
            filter_type = "claims.sub"

        # channel = self.membership_service_channel
        # accounts = None
        #
        # req = membership_service_pb2.ListMembersRequest(
        #     filter=f"{filter_type}=\"{account_id}\"",
        #     organization_id=organization_id
        # )
        #
        # try:
        #     accounts = channel.ListMembers(req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return accounts
        return self.use_grpc(self.membership_service_channel.ListMembers,
                             membership_service_pb2.ListMembersRequest(
                                 filter=f"{filter_type}=\"{account_id}\"",
                                 organization_id=organization_id
                             ))

    def account_get_by_login(self, account_id):
        human = False
        result = self.get_service_account(account_id)

        if result is None:
            result = self.get_user_account(account_id) or self.get_yandex_passport_user_account(account_id)
            human = True
        return result, human
