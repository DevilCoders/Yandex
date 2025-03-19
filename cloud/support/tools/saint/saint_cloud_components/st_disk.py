from grpc_gw import *



class St_Disk:

    def check_source_image(self, image_id):
        # img_channel = self.image_service_channel
        # img_req = image_service_pb2.GetImageRequest(image_id=image_id)
        # i = None
        # try:
        #     i = img_channel.Get(img_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return i
        return self.use_grpc(self.image_service_channel.Get,
                             image_service_pb2.GetImageRequest(image_id=image_id))

    def disk_get(self, disk_id):
        # disk_channel = self.disk_service_channel
        # disk_req = disk_service_pb2.GetDiskRequest(disk_id=disk_id)
        # res = None
        # try:
        #     res = disk_channel.Get(disk_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return res
        return self.use_grpc(self.disk_service_channel.Get,
                             disk_service_pb2.GetDiskRequest(disk_id=disk_id))
