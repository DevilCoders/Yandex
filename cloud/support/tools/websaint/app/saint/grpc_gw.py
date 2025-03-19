import os
import grpc
from grpc._channel import _InactiveRpcError
import logging

from yandex.cloud.priv.iam.v1 import iam_token_service_pb2_grpc as iam_token_service
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2 as iam_token_service_pb2
from yandex.cloud.priv.iam.v1 import user_account_service_pb2 as user_account_service_pb2
from yandex.cloud.priv.iam.v1 import user_account_service_pb2_grpc as user_account_service
from yandex.cloud.priv.iam.v1 import access_binding_service_pb2_grpc as access_service
from yandex.cloud.priv.iam.v1 import service_account_service_pb2_grpc as service_account_service
from yandex.cloud.priv.iam.v1 import service_account_service_pb2 as service_account_service_pb2
from yandex.cloud.priv.iam.v1 import yandex_passport_user_account_service_pb2_grpc as user_account_pub_service
from yandex.cloud.priv.iam.v1 import yandex_passport_user_account_service_pb2 as user_account_pub_service_pb2
from yandex.cloud.priv.iam.v1 import access_binding_service_pb2 as access_service_pb2
from yandex.cloud.priv.iam.v1.backoffice import access_binding_service_pb2_grpc as ab_service
from yandex.cloud.priv.iam.v1.backoffice import access_binding_service_pb2 as ab_service_pb2
from yandex.cloud.priv.iam.v1.console import membership_service_pb2 as membership_service_pb2
from yandex.cloud.priv.iam.v1.console import membership_service_pb2_grpc as membership_service

from yandex.cloud.priv.resourcemanager.v1 import cloud_service_pb2_grpc as cloud_service
from yandex.cloud.priv.resourcemanager.v1 import cloud_service_pb2 as cloud_service_pb2
from yandex.cloud.priv.resourcemanager.v1 import folder_service_pb2_grpc as folder_service
from yandex.cloud.priv.resourcemanager.v1 import folder_service_pb2 as folder_service_pb2
from yandex.cloud.priv.resourcemanager.v1.console import cloud_service_pb2_grpc as console_cloud_service
from yandex.cloud.priv.resourcemanager.v1.console import cloud_service_pb2 as console_cloud_service_pb2

from yandex.cloud.priv.console.v1 import resource_settings_pb2 as resource_settings_pb2

from yandex.cloud.priv.compute.v1 import image_service_pb2_grpc as image_service
from yandex.cloud.priv.compute.v1 import image_service_pb2 as image_service_pb2
from yandex.cloud.priv.compute.v1 import disk_service_pb2_grpc as disk_service
from yandex.cloud.priv.compute.v1 import disk_service_pb2 as disk_service_pb2
from yandex.cloud.priv.compute.v1 import operation_service_pb2_grpc as operation_service
from yandex.cloud.priv.compute.v1 import operation_service_pb2 as operation_service_pb2
from yandex.cloud.priv.compute.v1 import instance_service_pb2_grpc as instance_service
from yandex.cloud.priv.compute.v1 import instance_service_pb2 as instance_service_pb2

from yandex.cloud.priv.vpc.v1 import subnet_service_pb2_grpc as subnet_service
from yandex.cloud.priv.vpc.v1 import subnet_service_pb2 as subnet_service_pb2
from yandex.cloud.priv.vpc.v1 import network_service_pb2 as network_service_pb2
from yandex.cloud.priv.vpc.v1 import network_service_pb2_grpc as network_service
from yandex.cloud.priv.vpc.v1.inner import network_interface_attachment_service_pb2_grpc as na_service
from yandex.cloud.priv.vpc.v1.inner import network_interface_attachment_service_pb2 as na_service_pb2
from yandex.cloud.priv.vpc.v1.inner import backoffice_service_pb2 as backoffice_service_pb2
from yandex.cloud.priv.vpc.v1.inner import backoffice_service_pb2_grpc as backoffice_service
from yandex.cloud.priv.vpc.v1.inner import address_service_pb2_grpc as address_service
from yandex.cloud.priv.vpc.v1.inner import address_service_pb2 as address_service_pb2


from yandex.cloud.priv.k8s.v1.inner import cluster_service_pb2_grpc as cluster_service
from yandex.cloud.priv.k8s.v1.inner import cluster_service_pb2 as cluster_service_pb2


def get_grpc_channel(service_stub, endpoint, iam_token=""):
    """
    get encrypted grpc channel from stub, token and endpoint
    :param service_stub: grpc service stub
    :param endpoint: grpc endpoint string
    :param iam_token: string
    :return:
    """
    ssl = os.environ['REQUESTS_CA_BUNDLE']
    with open(ssl, 'rb') as cert:
        ssl_creds = grpc.ssl_channel_credentials(cert.read())
    call_creds = grpc.access_token_call_credentials(iam_token)
    chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

    stub = service_stub  # i.e.: folder_service.FolderServiceStub
    channel = grpc.secure_channel(endpoint, chan_creds)

    return stub(channel)


def get_grpc_pub_channel(service_stub, endpoint, iam_token=""):
    """
    get encrypted grpc channel from stub, token and endpoint
    :param service_stub: grpc service stub
    :param endpoint: grpc endpoint string
    :param iam_token: string
    :return:
    """
    # ssl = os.environ['REQUESTS_CA_BUNDLE']
    ssl = '/home/the-nans/arcadia/junk/the-nans/saint/CA.pem'
    with open(ssl, 'rb') as cert:
        ssl_creds = grpc.ssl_channel_credentials(cert.read())
    call_creds = grpc.access_token_call_credentials(iam_token)
    chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

    stub = service_stub  # i.e.: folder_service.FolderServiceStub
    channel = grpc.secure_channel(endpoint, chan_creds)

    return stub(channel)


class GrpcChannels:

    @property
    def cloud_service_channel(self):
        print(self.iam_token)
        channel = get_grpc_channel(cloud_service.CloudServiceStub, self.endpoints.cloud_url[self.profile.name.upper()],
                                   self.iam_token)
        return channel

    @property
    def cloud_console_service_channel(self):
        channel = get_grpc_channel(console_cloud_service.CloudServiceStub,
                                   self.endpoints.cloud_url[self.profile.name.upper()], self.iam_token)
        return channel

    @property
    def access_binding_service_channel(self):
        iam_token = self.iam_token
        return get_grpc_channel(access_service.AccessBindingServiceStub,
                                self.endpoints.iam_url[self.profile.name.upper()], iam_token)

    @property
    def folder_service_channel(self):
        iam_token = self._get_cached_token()
        endpoint = self.endpoints.search_folder_url[self.profile.name.upper()]

        return get_grpc_channel(folder_service.FolderServiceStub, endpoint, iam_token)

    @property
    def image_service_channel(self):
        iam_token = self.iam_token
        endpoint = self.endpoints.compute_api[self.profile.name.upper()]
        return get_grpc_channel(image_service.ImageServiceStub,
                                endpoint, iam_token)

    @property
    def disk_service_channel(self):
        iam_token = self.iam_token
        endpoint = self.endpoints.compute_api[self.profile.name.upper()]
        return get_grpc_channel(disk_service.DiskServiceStub,
                                endpoint, iam_token)

    @property
    def instance_service_channel(self):
        iam_token = self.iam_token
        endpoint = self.endpoints.compute_api[self.profile.name.upper()]
        return get_grpc_channel(instance_service.InstanceServiceStub,
                                endpoint, iam_token)

    @property
    def network_interface_attachment_channel(self):
        return get_grpc_channel(na_service.NetworkInterfaceAttachmentServiceStub,
                                self.profile.endpoints.network_url[self.profile.name.upper()], self.iam_token)

    @property
    def cluster_service_channel(self):
        return get_grpc_channel(cluster_service.ClusterServiceStub,
                                self.endpoints.k8s_url[self.profile.name.upper()], self.iam_token)

    @property
    def address_service_channel(self):
        endpoint = self.endpoints.network_url[self.profile.name.upper()]
        return get_grpc_channel(address_service.AddressServiceStub,
                                endpoint, self.iam_token)

    @property
    def backoffice_service_channel(self):
        endpoint = self.endpoints.network_url[self.profile.name.upper()]
        return get_grpc_channel(backoffice_service.BackofficeServiceStub,
                                endpoint, self.iam_token)

    @property
    def network_service_channel(self):
        return get_grpc_channel(network_service.NetworkServiceStub,
                                self.endpoints.network_url[self.profile.name.upper()], self.iam_token)

    @property
    def subnet_service_channel(self):
        endpoint = self.endpoints.network_url[self.profile.name.upper()]
        return get_grpc_channel(subnet_service.SubnetServiceStub,
                                endpoint, self.iam_token)

    @property
    def user_account_service_channel(self):
        endpoint = self.endpoints.iam_url[self.profile.name.upper()]
        return get_grpc_channel(user_account_service.UserAccountServiceStub,
                                endpoint, self.iam_token)

    @property
    def yandex_passport_user_account_service_channel(self):
        endpoint = self.endpoints.iam_url[self.profile.name.upper()]
        return get_grpc_channel(user_account_pub_service.YandexPassportUserAccountServiceStub,
                                endpoint, self.iam_token)

    @property
    def backoffice_access_bindings_service_channel(self):
        endpoint = self.endpoints.iam_url[self.profile.name.upper()]
        return get_grpc_channel(ab_service.AccessBindingServiceStub, endpoint,
                                self.iam_token)

    @property
    def service_account_channel(self):
        endpoint = self.endpoints.iam_url[self.profile.name.upper()]
        return get_grpc_channel(service_account_service.ServiceAccountServiceStub,
                                endpoint, self.iam_token)

    @property
    def membership_service_channel(self):
        return get_grpc_channel(membership_service.MembershipServiceStub,
                                self.endpoints.iam_url[self.profile.name.upper()], self.iam_token)

    @staticmethod
    def use_grpc(channel, request):
        result = None
        try:
            result = channel(request)
        except _InactiveRpcError as e:
            print(e.details())
            logging.debug(e.details())

        return result
