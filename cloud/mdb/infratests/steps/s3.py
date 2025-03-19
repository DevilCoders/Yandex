from urllib.parse import urlparse

from behave import then

from cloud.mdb.infratests.test_helpers import s3
from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.utils import render_text


@then('s3 file {uri} contains')
def step_check_s3_file_contains(context: Context, uri: str):
    uri = render_text(context, uri)
    if not uri.startswith("s3://"):
        raise Exception(f'Unsupported uri: {uri}')

    parsed_uri = urlparse(uri)
    bucket_name = parsed_uri.netloc
    object_name = parsed_uri.path[1:]
    file_content = s3.download_object(context, bucket_name, object_name)

    assert context.text in file_content, 'Not found text {body} within s3 file {uri}'.format(body=context.text, uri=uri)


@then('combined content of files within s3 folder {uri} contains')
def step_check_files_within_s3_folder_contain(context: Context, uri: str):
    uri = render_text(context, uri)
    if not uri.startswith("s3://"):
        raise Exception(f'Unsupported uri: {uri}')

    parsed_uri = urlparse(uri)
    bucket_name = parsed_uri.netloc
    folder_name = parsed_uri.path[1:]
    combined_content = s3.combined_content(context, bucket_name, folder_name)

    assert context.text in combined_content, 'Not found text {body} within files in s3 folder {uri}'.format(
        body=context.text, uri=uri
    )
