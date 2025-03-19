from app.saint.grpc_gw import *

class St_Operation:

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



