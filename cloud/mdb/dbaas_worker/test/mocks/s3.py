"""
Simple s3 mock
"""

from types import SimpleNamespace

from botocore.exceptions import ClientError

from .utils import handle_action


def create_bucket(state, Bucket):
    """
    Create bucket mock
    """
    action_id = f's3-create-{Bucket}'
    handle_action(state, action_id)
    if Bucket in state['s3']:
        raise RuntimeError(f'Bucket {Bucket} already exists')

    state['s3'][Bucket] = {'files': [], 'uploads': []}


def filter_objects(state, Prefix):
    """
    Filter objects (single bucket expected)
    """
    action_id = f's3-filter-{Prefix}'
    handle_action(state, action_id)
    for file in next(iter(state['s3'].values()))['files']:
        if file.startswith(Prefix):
            yield SimpleNamespace(key=file)


def delete_objects(state, Delete):
    """
    Delete objects (single bucket expected)
    """
    action_id = 's3-delete'
    handle_action(state, action_id)
    for obj in Delete['Objects']:
        next(iter(state['s3'].values()))['files'].remove(obj['Key'])
    return dict()


def head_bucket(state, Bucket):
    """
    Head bucket mock
    """
    action_id = f's3-head-bucket-{Bucket}'
    handle_action(state, action_id)
    if Bucket not in state['s3']:
        raise ClientError({'Error': {'Code': 404}}, 'head_bucket')


def list_multipart_uploads(state, Bucket):
    """
    List multipart uploads mock
    """
    action_id = f's3-list-multipart-uploads-{Bucket}'
    handle_action(state, action_id)
    return {'Uploads': state['s3'][Bucket]['uploads']}


def abort_multipart_upload(state, Bucket, Key, UploadId):
    """
    Abort multipart upload mock
    """
    action_id = f's3-abort-multipart-upload-{Bucket}-{Key}-{UploadId}'
    handle_action(state, action_id)
    value = None
    for upload in state['s3'][Bucket]['uploads']:
        if upload['Key'] == Key and upload['UploadId'] == UploadId:
            value = upload
            break

    if not value:
        raise RuntimeError(f'Multipart upload for {Key} with id {UploadId} not found')

    state['s3'][Bucket]['uploads'].remove(value)


def delete_bucket(state, Bucket):
    """
    Delete bucket mock
    """
    action_id = f's3-delete-{Bucket}'
    handle_action(state, action_id)
    if Bucket not in state['s3']:
        raise RuntimeError(f'Bucket {Bucket} not found')
    if state['s3'][Bucket]['uploads']:
        raise RuntimeError(f'Bucket {Bucket} has unfinished multipart uploads')
    if state['s3'][Bucket]['files']:
        raise RuntimeError(f'Bucket {Bucket} has files')

    del state['s3'][Bucket]


def s3(mocker, state):
    """
    Setup s3 mock
    """
    resource = mocker.Mock()
    session = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.s3_bucket.boto3.Session')
    session.return_value.resource.return_value = resource

    resource.create_bucket.side_effect = lambda Bucket: create_bucket(state, Bucket)
    resource.Bucket.return_value.objects.filter.side_effect = lambda Prefix: filter_objects(state, Prefix)
    resource.Bucket.return_value.delete_objects.side_effect = lambda Delete: delete_objects(state, Delete)
    resource.meta.client.head_bucket.side_effect = lambda Bucket: head_bucket(state, Bucket)
    resource.meta.client.list_multipart_uploads.side_effect = lambda Bucket: list_multipart_uploads(state, Bucket)
    resource.meta.client.abort_multipart_upload.side_effect = lambda Bucket, Key, UploadId: abort_multipart_upload(
        state, Bucket, Key, UploadId
    )
    resource.meta.client.delete_bucket.side_effect = lambda Bucket: delete_bucket(state, Bucket)
