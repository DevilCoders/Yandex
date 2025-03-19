from grpc_gw import *


class St_Operation:

    def list_compute_host_operations(self, fqdn):
        return self.use_grpc(self.compute_host_service_channel.ListOperations,
                             compute_host_service_pb2.ListNodeOperationsRequest(fqdn=fqdn)).operations

    def instance_operations_list(self, instance_id=''):
        # op_channel = self.instance_service_channel
        # op_req = instance_service_pb2.ListInstanceOperationsRequest(instance_id=instance_id, page_size=1000)
        # operations = None
        # try:
        #     operations = op_channel.ListOperations(op_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return operations.operations
        return self.use_grpc(self.instance_service_channel.ListOperations,
                             instance_service_pb2.ListInstanceOperationsRequest(instance_id=instance_id,
                                                                                page_size=1000)).operations

    def folder_operations_list(self, folder_id=''):
        return self.use_grpc(self.folder_service_channel.ListOperations,
                             folder_service_pb2.ListFolderOperationsRequest(folder_id=folder_id,
                                                                            page_size=1000)).operations
