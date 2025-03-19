from grpc_gw import *

class St_Folder:

    def get_folder_by_id(self, folder_id):
        # channel = self.folder_service_channel
        # resp = None
        # req = folder_service_pb2.GetFolderRequest(
        #     folder_id=folder_id
        # )
        # try:
        #     resp = channel.Get(req)
        # except _InactiveRpcError as e:
        #     logging.debug(e.details())
        #
        # return resp
        #
        return self.use_grpc(self.folder_service_channel.Get,
                             folder_service_pb2.GetFolderRequest(
                                 folder_id=folder_id
                             ))

    def folder_images_list(self, folder_id='standard-images'):

        # img_channel = self.image_service_channel
        # img_req = image_service_pb2.ListImagesRequest(folder_id=folder_id, page_size=1000)
        #
        # img = None
        # try:
        #     img = img_channel.List(img_req)
        # except _InactiveRpcError as e:
        #     logging.debug(e)
        #
        # return img
        return self.use_grpc(self.image_service_channel.List,
                             image_service_pb2.ListImagesRequest(folder_id=folder_id, page_size=1000))
