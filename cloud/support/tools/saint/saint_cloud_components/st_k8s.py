from grpc_gw import *


class St_K8s:



    def gmk8s_master_list(self, cluster_id):
        """
        уродливо ходит в фолдер с мастерами и грепает их инстансы по имени
        :param cluster_id:
        :return:
        """
        # instances_channel = self.instance_service_channel
        result = []
        for c_id_var in ['-a', '-b', '-c', '']:
            k8s_masters = self.use_grpc(self.instance_service_channel.List,
                          instance_service_pb2.ListInstancesRequest(view='BASIC', folder_id='b1g1d7mruu11c2fpaa1b',
                                                                    filter=f'name="{cluster_id}{c_id_var}"'))

            # req = instance_service_pb2.ListInstancesRequest(view='BASIC', folder_id='b1g1d7mruu11c2fpaa1b',
            #                                                 filter=f'name="{cluster_id}{c_id_var}"')
            # try:
            #     k8s_masters = instances_channel.List(req)
            #     if len(k8s_masters.instances) > 0:
            #         result.append(k8s_masters.instances[0])
            # except _InactiveRpcError as e:
            #     logging.debug(e.details())
            if len(k8s_masters.instances) > 0:
                result.append(k8s_masters.instances[0])
        return result

