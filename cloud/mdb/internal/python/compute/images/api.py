from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, Generator, Iterable, NamedTuple
from yandex.cloud.priv.compute.v1 import image_service_pb2, image_service_pb2_grpc

from cloud.mdb.internal.python.compute.pagination import ComputeResponse, paginate
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import ImageModel


class ImagesClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class ImagesClient:
    __image_service = None

    def __init__(
        self,
        config: ImagesClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='ImagesClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _image_service(self):
        if self.__image_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__image_service = ComputeGRPCService(
                self.logger,
                channel,
                image_service_pb2_grpc.ImageServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__image_service

    @client_retry
    @tracing.trace('Compute List Images page')
    def _list_images(self, request: image_service_pb2.ListImagesRequest) -> Iterable:
        tracing.set_tag('compute.folder.id', request.folder_id)
        response = self._image_service.List(request)
        return ComputeResponse(
            resources=map(ImageModel.from_api, response.images),
            next_page_token=response.next_page_token,
        )

    def list_images(self, folder_id: str) -> Generator[ImageModel, None, None]:
        """
        Get disks in folder
        """
        request = image_service_pb2.ListImagesRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_images, request)

    @client_retry
    @tracing.trace('Compute Get Image')
    def get_image(self, image_id: str) -> ImageModel:
        """
        Get image info
        """
        tracing.set_tag('compute.image.id', image_id)
        request = image_service_pb2.GetImageRequest()
        request.image_id = image_id
        return ImageModel.from_api(self._image_service.Get(request))
