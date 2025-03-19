import functools
import grpc
import hashlib
import json
import six

from google.protobuf.wrappers_pb2 import BoolValue, StringValue

from yc_snapshot_client.snapshot_rpc import snapshot_python_pb2 as pb2
from yc_snapshot_client.snapshot_rpc import snapshot_python_pb2_grpc as pb2grpc
from yc_snapshot_client.snapshot_rpc.snapshot_python_pb2 import \
    SnapshotMoveSrc, SnapshotMoveDst, NbsMoveSrc, NbsMoveDst, \
    NullMoveSrc, NullMoveDst, UrlMoveSrc, MoveParams, DeleteParams, \
    CopyParams


YC_METADATA_PREFIX = "yc-"


class ErrorCode(object):
    UNKNOWN = "ErrUnknown"
    INTERNAL = "ErrInternal"
    DUPLICATE_NAME = "ErrDuplicateName"
    DUPLICATE_ID = "ErrDuplicateID"
    NOT_FOUND = "ErrSnapshotNotFound"
    INVALID_FORMAT = "ErrInvalidFormat"  # Wrong source image format
    UNKNOWN_SOURCE = "ErrUnknownSource"  # Wrong url format or unknown scheme
    SOURCE_NO_RANGE = "ErrSourceNoRange"  # Source HTTP server does not support byte ranges
    UNREACHABLE_SOURCE = "ErrUnreachableSource"  # Failed to HEAD source
    INVALID_OBJECT = "ErrInvalidObject"  # For S3
    TASK_NOT_FOUND = "ErrTaskNotFound"  # Task processing
    DUPLICATE_TASK_ID = "ErrDuplicateTaskID"  # Task processing
    NBS = "ErrNbs"  # Nbs errors


class SnapshotClientError(RuntimeError):
    def __init__(self, code, message):
        self.code = code
        self.message = message

    @classmethod
    def from_rpc_error(cls, rpc_error):
        try:
            d = json.loads(rpc_error.details())
            return cls(d["code"], d["message"])
        except Exception:
            return cls(ErrorCode.UNKNOWN, str(rpc_error))

    @classmethod
    def from_json(cls, details):
        try:
            d = json.loads(details)
            return cls(d["code"], d["message"])
        except Exception:
            return cls(ErrorCode.UNKNOWN, str(details))

    def __str__(self):
        return "SnapshotClientError: {} {}".format(self.code, self.message)


class UpdateableMetadata(object):
    def __init__(self, metadata="", description=""):
        self.metadata = metadata
        self.description = description


class CreateMetadata(UpdateableMetadata):
    def __init__(self, metadata="", organization="", project_id="",
                 name="", description="", product_id="", image_id="",
                 public=False, snapshot_id=""):
        super(CreateMetadata, self).__init__(
                metadata=metadata, description=description)
        self.organization = organization
        self.project_id = project_id
        self.name = name
        self.product_id = product_id
        self.image_id = image_id
        self.public = public
        self.id = snapshot_id

    def to_req(self, bucket, key, url, format):
        return pb2.ConvertRequest(bucket=bucket, key=key, url=url,
                                  format=format, **self.__dict__)


class CreateInfo(CreateMetadata):
    def __init__(self, base="", base_project_id="", size=0, metadata="",
                 disk="", organization="", project_id="", name="", description="",
                 product_id="", image_id="", public=False, snapshot_id=""):
        super(CreateInfo, self).__init__(
                metadata=metadata,
                organization=organization,
                project_id=project_id,
                name=name,
                description=description,
                product_id=product_id,
                image_id=image_id,
                public=public,
                snapshot_id=snapshot_id,
        )
        self.base = base
        self.base_project_id = base_project_id
        self.size = size
        self.disk = disk

    def to_req(self):
        return pb2.CreateRequest(**self.__dict__)


class UpdateInfo:
    def __init__(self, snapshot_id, metadata=None, description=None, public=None):
        self.id = snapshot_id

        if metadata is not None:
            self.metadata = StringValue(value=metadata)

        if description is not None:
            self.description = StringValue(value=description)

        if public is not None:
            self.public = BoolValue(value=public)

    def to_req(self):
        return pb2.UpdateRequest(**self.__dict__)


class CopyRequest(object):
    def __init__(self, source_id="", target_id="", target_project_id="",
                 name="", image_id="", copy_params=None):
        self.id = source_id
        self.target_id = target_id
        self.target_project_id = target_project_id
        self.name = name
        self.image_id = image_id
        self.params = copy_params

    def to_req(self):
        return pb2.CopyRequest(**self.__dict__)


class MoveRequest(object):
    def __init__(self,
                 snapshot_move_src=None,
                 nbs_move_src=None,
                 null_move_src=None,
                 url_move_src=None,
                 snapshot_move_dst=None,
                 nbs_move_dst=None,
                 null_move_dst=None,
                 move_params=None):

        if ((snapshot_move_src is not None) +
                (nbs_move_src is not None) +
                (null_move_src is not None) +
                (url_move_src is not None) != 1):
            raise SnapshotClientError(ErrorCode.UNKNOWN, "one source must be specified")

        if ((snapshot_move_dst is not None) +
                (nbs_move_dst is not None) +
                (null_move_dst is not None) != 1):
            raise SnapshotClientError(ErrorCode.UNKNOWN, "one destination must be specified")

        self.snapshot_src = snapshot_move_src
        self.nbs_src = nbs_move_src
        self.null_src = null_move_src
        self.url_src = url_move_src

        self.snapshot_dst = snapshot_move_dst
        self.nbs_dst = nbs_move_dst
        self.null_dst = null_move_dst

        self.params = move_params

    def to_req(self):
        return pb2.MoveRequest(**self.__dict__)


class StatusWrapper(object):
    def __init__(self, message):
        self.message = message

    def details(self):
        return self.message

    def __repr__(self):
        return "StatusWrapper: {}".format(self.message)


class TaskStatus(object):
    def __init__(self, proto):
        self.finished = proto.finished
        self.success = proto.success
        self.progress = proto.progress
        self.offset = proto.offset
        self.error = SnapshotClientError.from_json(proto.error.message) \
            if proto.error.code != 0 else None
        self.proto = proto

    def __repr__(self):
        return str(self.__dict__)


class DeleteRequest(object):
    def __init__(self, snapshot_id, skip_status_check=False, delete_params=None):
        self.id = snapshot_id
        self.skip_status_check = skip_status_check
        self.params = delete_params

    def to_req(self):
        return pb2.DeleteRequest(**self.__dict__)


def _handle_errors(f):
    @functools.wraps(f)
    def wrapper(*args, **kw):
        try:
            return f(*args, **kw)
        except grpc.RpcError as e:
            raise SnapshotClientError.from_rpc_error(e)
        # We do not catch standard errors like ValueError, EOFError

    return wrapper


def _prepare_metadata(metadata):
    if isinstance(metadata, dict):
        metadata = [(YC_METADATA_PREFIX + str(k), str(v)) for k, v in metadata.items()]
    elif isinstance(metadata, list) or isinstance(metadata, tuple):
        metadata = [(YC_METADATA_PREFIX + str(k), str(v)) for k, v in metadata]
    elif metadata is not None:
        raise SnapshotClientError(ErrorCode.UNKNOWN, "metadata must be list of 2-tuples or dict")

    return metadata


def _build_metadata(metadata=None, metadata_callback=None):
    if metadata is None and metadata_callback is not None:
        metadata = metadata_callback()

    return _prepare_metadata(metadata)


class SnapshotClient(object):
    """
    Must use with context manager for prevent grpc connection leak.
    """

    @_handle_errors
    def __init__(self, target, credentials=None, options=None, timeout=None, metadata_callback=None):
        # FIXME: Move initialization to __enter__ while existed code will rewrite to context manager
        self._channel = None
        if credentials is None:
            self._channel = grpc.insecure_channel(target, options)
        else:
            self._channel = grpc.secure_channel(target, credentials, options)
        self._stub = pb2grpc.SnapshotServiceStub(self._channel)
        self._timeout = timeout
        self._metadata_callback = metadata_callback

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close_connection()

    def close_connection(self):
        if self._channel is not None:
            self._channel.close()
            self._channel = None

    @_handle_errors
    def open(self, snapshot_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        stream = self._stub.GetMap(pb2.MapRequest(id=snapshot_id),
                                   timeout=timeout, metadata=metadata)
        cmap = {}
        last_size = None
        last_offset = -1
        chunkhasher = hashlib.sha512()
        for item in stream:
            for chunk in item.chunks:
                if last_size is not None and last_size != chunk.chunksize:
                    raise ValueError("incorrect size %d for chunk %s" %
                                     (chunk.chunksize, chunk.hashsum))
                if last_offset >= chunk.offset:
                    raise ValueError("offset is not in"
                                     "increasing order %s" % chunk.hashsum)
                last_size = chunk.chunksize
                last_offset = int(chunk.offset)
                cmap[last_offset] = chunk
                chunkhasher.update(six.b(chunk.hashsum))

        checksum = chunkhasher.name + ":" + chunkhasher.hexdigest()
        if len(cmap) == 0:
            raise ValueError("empty snapshot")
        return SnapshotReader(self._stub, self._timeout, self._metadata_callback,
                              snapshot_id, cmap, checksum)

    @_handle_errors
    def commit(self, snapshot_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Commit(pb2.CommitRequest(id=snapshot_id),
                                 timeout=timeout, metadata=metadata)

    @_handle_errors
    def delete(self, delete_request, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Delete(delete_request.to_req(),
                                 timeout=timeout, metadata=metadata)

    @_handle_errors
    def create(self, create_info, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Create(create_info.to_req(), timeout=timeout, metadata=metadata).id

    @_handle_errors
    def write(self, snapshot_id, data, offset, progress=float(0), timeout=None, metadata=None):
        return self.write_async(snapshot_id, data, offset, progress=progress,
                                timeout=timeout, metadata=metadata).result()

    def write_async(self, snapshot_id, data, offset, progress=float(0), timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        coord = pb2.Coord(offset=offset, chunksize=len(data))
        req = pb2.WriteRequest(id=snapshot_id, coord=coord, data=data, progress=progress)
        return self._stub.Write.future(req, timeout=timeout, metadata=metadata)

    @staticmethod
    @_handle_errors
    def write_result(future):
        return future.result()

    @_handle_errors
    def list(self, **kwargs):
        timeout = kwargs.pop("timeout", None)
        timeout = timeout if timeout is not None else self._timeout
        metadata = kwargs.pop("metadata", None)
        metadata = _build_metadata(metadata, self._metadata_callback)
        req = pb2.ListRequest(**kwargs)
        listresp = self._stub.List(req, timeout=timeout, metadata=metadata)
        return listresp.result, listresp.has_more, listresp.next_cursor

    @_handle_errors
    def list_full(self, **kwargs):
        if "limit" in kwargs or "sort" in kwargs:
            raise ValueError("limit and sort parameters cannot be used in list_full")

        total = []
        while True:
            listing, _, cursor = self.list(**kwargs)
            total.extend(listing)
            if not cursor:
                break
            kwargs["cursor"] = cursor
        return total

    @_handle_errors
    def list_changed_children(self, snapshot_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.ListChangedChildren(
            pb2.ChangedChildrenListRequest(id=snapshot_id),
            timeout=timeout, metadata=metadata).changed_children

    @_handle_errors
    def list_bases(self, snapshot_id, limit=0, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.ListBases(pb2.BaseListRequest(id=snapshot_id, limit=limit),
                                    timeout=timeout, metadata=metadata).result

    @_handle_errors
    def info(self, snapshot_id, skip_status_check=False, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Info(pb2.InfoRequest(id=snapshot_id, skip_status_check=skip_status_check),
                               timeout=timeout, metadata=metadata)

    @_handle_errors
    def convert(self, create_metadata, bucket="", key="", url="", fmt="", timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Convert(create_metadata.to_req(bucket, key, url, fmt),
                                  timeout=timeout, metadata=metadata).id

    @_handle_errors
    def update(self, update_info, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Update(update_info.to_req(), timeout=timeout, metadata=metadata)

    @_handle_errors
    def verify(self, snapshot_id, full=False, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Verify(pb2.VerifyRequest(id=snapshot_id, full=full),
                                 timeout=timeout, metadata=metadata)

    @_handle_errors
    def copy(self, copy_request, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Copy(copy_request.to_req(), timeout=timeout, metadata=metadata)

    @_handle_errors
    def move(self, move_request, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.Move(move_request.to_req(), timeout=timeout, metadata=metadata)

    @_handle_errors
    def get_task_status(self, task_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return TaskStatus(self._stub.GetTaskStatus(pb2.GetTaskStatusRequest(task_id=task_id),
                                                   timeout=timeout, metadata=metadata))

    @_handle_errors
    def cancel_task(self, task_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.CancelTask(pb2.CancelTaskRequest(task_id=task_id),
                                     timeout=timeout, metadata=metadata)

    @_handle_errors
    def delete_task(self, task_id, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        return self._stub.DeleteTask(pb2.DeleteTaskRequest(task_id=task_id),
                                     timeout=timeout, metadata=metadata)


class SnapshotReader(object):
    def __init__(self, stub, timeout, metadata_callback, snapshot_id,
                 snapshot_map, checksum):
        self._stub = stub
        self._timeout = timeout
        self._metadata_callback = metadata_callback
        self._snapshot_id = snapshot_id
        self._snapshot_map = snapshot_map
        self._checksum = checksum
        self._chunksize = next(six.itervalues(snapshot_map)).chunksize

    @_handle_errors
    def read(self, size, offset, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        buf = []
        while size > 0:
            item = self._snapshot_map.get(offset / self._chunksize * self._chunksize)
            if item is None:
                if not buf:
                    raise EOFError("no more data")
                else:
                    return six.b("").join(buf)

            length = min(self._chunksize - offset % self._chunksize, size)
            if item.zero:
                buf.append(six.b("\x00") * length)
            else:
                coord = pb2.Coord(offset=item.offset, chunksize=item.chunksize)
                resp = self._stub.Read(pb2.ReadRequest(
                    id=self._snapshot_id, coord=coord),
                    timeout=timeout, metadata=metadata)
                buf.append(resp.data[offset % self._chunksize: offset % self._chunksize + length])
            offset += length
            size -= length

        return six.b("").join(buf)

    def read_block_async(self, size, offset, timeout=None, metadata=None):
        timeout = timeout if timeout is not None else self._timeout
        metadata = _build_metadata(metadata, self._metadata_callback)
        # Check arguments
        self._get_block_from_map(size, offset)

        coord = pb2.Coord(offset=offset, chunksize=size)
        req = pb2.ReadRequest(id=self._snapshot_id, coord=coord)
        return self._stub.Read.future(req, timeout=timeout, metadata=metadata)

    @staticmethod
    @_handle_errors
    def read_block_result(future):
        return future.result().data

    def _get_block_from_map(self, size, offset):
        if offset % self._chunksize != 0:
            raise ValueError("Offset must be a multiple of {}".format(self._chunksize))

        if size != self._chunksize:
            raise ValueError("Size must be equal to {}".format(self._chunksize))

        item = self._snapshot_map.get(offset)
        if item is None:
            raise ValueError("Block is after snapshot end")

        return item

    def is_block_zero(self, size, offset):
        item = self._get_block_from_map(size, offset)
        return item.zero

    def get_block_checksum(self, size, offset):
        item = self._snapshot_map.get(offset)
        if item is None:
            raise IndexError("no such offset")

        if item.chunksize != size:
            raise ValueError("size does not match")
        return item.hashsum

    @property
    def snapshot_checksum(self):
        return self._checksum
