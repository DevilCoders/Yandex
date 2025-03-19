from grpc_gw import *


class St_ComputeHost:
    def get_compute_host(self, fqdn):
        return self.use_grpc(self.compute_host_service_channel.Get,
                             compute_host_service_pb2.GetNodeRequest(fqdn=fqdn))

    def list_compute_host_instances(self, fqdn):
        return self.use_grpc(self.compute_host_service_channel.ListInstances,
                             compute_host_service_pb2.ListNodeInstancesRequest(fqdn=fqdn))
