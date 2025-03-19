from grpc_gw import *


class St_Network:


    def get_address_by_id(self, address_id):
        # addr_channel = self.address_service_channel
        #
        # address_raw = None
        # try:
        #     addr_req = address_service_pb2.GetAddressRequest(address_id=address_id)
        #     address_raw = addr_channel.Get(addr_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return address_raw
        return self.use_grpc(self.address_service_channel.Get,
                             address_service_pb2.GetAddressRequest(address_id=address_id))

    def get_address_by_ip(self, ip):
        result = None

        # by_ip_channel = self.backoffice_service_channel
        # by_ip_req = backoffice_service_pb2.FindAddressRequest(address=ip)
        #
        # try:
        #     subject = by_ip_channel.FindAddress(by_ip_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())

        subject = self.use_grpc(self.backoffice_service_channel.FindAddress,
                                backoffice_service_pb2.FindAddressRequest(address=ip))
        if subject:
            result = self.get_address_by_id(subject.address_id)

        return result

    def get_network(self, network_id):

        # net_channel = self.network_service_channel
        # net_req = network_service_pb2.GetNetworkRequest(network_id=network_id)
        # network = None
        # try:
        #     network = net_channel.Get(net_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return network
        return self.use_grpc(self.network_service_channel.Get,
                             network_service_pb2.GetNetworkRequest(network_id=network_id))

    def list_security_groups(self, network_id):
        # sg_gw = self.network_service_channel
        # sg_req = network_service_pb2.ListNetworkSecurityGroupsRequest(network_id=network_id)
        # sg = None
        # try:
        #     sg = sg_gw.ListSecurityGroups(sg_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return sg
        return self.use_grpc(self.network_service_channel.ListSecurityGroups,
                             network_service_pb2.ListNetworkSecurityGroupsRequest(network_id=network_id))

    def list_folder_ips(self, folder_id):
        # addresses_channel = self.address_service_channel
        # addresses_req = address_service_pb2.ListAddressesRequest(folder_id=folder_id, page_size=1000)
        # addr_list = None
        # try:
        #     addr_list = addresses_channel.List(addresses_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return addr_list
        return self.use_grpc(self.address_service_channel.List,
                             address_service_pb2.ListAddressesRequest(folder_id=folder_id, page_size=1000))

    def addresses_list_by_cloud(self, cloud_id):
        final = []
        for folder in self.folders_get_by_cloud(cloud_id).folders:
            folder_id = folder.id
            addresses = self.list_folder_ips(folder_id)
            for net in addresses.addresses:
                final.append(net)
        return final


    def subnet_get(self, subnet_id):
        # subnet_channel = self.subnet_service_channel
        # subnet_req = subnet_service_pb2.GetSubnetRequest(subnet_id=subnet_id)
        # subnet = None
        # try:
        #     subnet = subnet_channel.Get(subnet_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return subnet
        return self.use_grpc(self.subnet_service_channel.Get,
                             subnet_service_pb2.GetSubnetRequest(subnet_id=subnet_id))

    def list_folder_subnets(self, folder_id):
        # sn_channel = self.subnet_service_channel
        # sn_req = subnet_service_pb2.ListSubnetsRequest(folder_id=folder_id, page_size=1000)
        # subnets_raw = None
        # try:
        #     subnets_raw = sn_channel.List(sn_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        # return subnets_raw
        return self.use_grpc(self.subnet_service_channel.List,
                             subnet_service_pb2.ListSubnetsRequest(folder_id=folder_id, page_size=1000))

    def subnets_list_by_cloud(self, cloud_id):

        for folder in self.folders_get_by_cloud(cloud_id).folders:
            folder_id = folder.id
            res = self.list_folder_subnets(folder_id)
        return res


