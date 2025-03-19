from grpc_gw import *


class St_Instance:

    def get_instance(self, instance_id, view="BASIC"):
        # i_req = instance_service_pb2.GetInstanceRequest(instance_id=instance_id, view=view)
        # i_channel = self.instance_service_channel
        #
        # instance = None
        # try:
        #     instance = i_channel.Get(i_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e)
        # return instance
        return self.use_grpc(self.instance_service_channel.Get,
                             instance_service_pb2.GetInstanceRequest(instance_id=instance_id, view=view))

    def get_compute_node(self, instance_id):
        """
        returns instance's parent compute node
        :param instance_id: instance_id
        :return: parent compute node fqdn
        """
        r = self.get_instance(instance_id)
        return r.compute_node

    # def format_security_groups(self, instance):
    #
    #     sgs = list(instance.network_interfaces[i].security_group_ids)
    #     if sgs:
    #         security_groups = '\n'.join(sgs)
    #     elif not sgs and def_sg:
    #         security_groups = f'{def_sg}{c_blue}(default){c_end}'
    #     elif (not sgs) and (not def_sg):
    #         security_groups = '-'


    def get_network_attachments(self, instance_id, folder_id):

        # na_channel = self.network_interface_attachment_channel
        # na_subreq = na_service_pb2.InstanceIdRequest(instance_id=instance_id, folder_id=folder_id)
        # na_req = na_service_pb2.GetNetworkInterfaceAttachmentsRequest(instance_id_requests=[na_subreq, ])
        # network_attachments = None
        #
        # try:
        #     network_attachments = na_channel.Get(na_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return network_attachments.network_interface_attachments
        return self.use_grpc(self.network_interface_attachment_channel.Get,
                             na_service_pb2.GetNetworkInterfaceAttachmentsRequest(
                                 instance_id_requests=[na_service_pb2.InstanceIdRequest(instance_id=instance_id,
                                                                                        folder_id=folder_id), ]))

    def get_vcpu(self, instance_id):
        instance = self.get_instance(instance_id)
        cores_count = instance.resources.cores
        return cores_count

    def get_instances_disks(self, instance):

        disk_list = []

        if instance.boot_disk.disk_id != '':
            disk_list.append(
                {
                    'type': 'boot',
                    'disk': instance.boot_disk
                }
            )

            for x in instance.secondary_disks:
                disk_list.append({
                    'disk': x,
                    'type': 'secondary'
                })
        return disk_list
