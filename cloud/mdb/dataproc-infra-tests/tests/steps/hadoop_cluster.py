"""
Steps related to Hadoop cluster nodes.
"""
import pickle
from urllib.parse import urlparse

from behave import given, then, when

from tests.helpers import internal_api, s3
from tests.helpers.hadoop_cluster import HadoopCluster
from tests.helpers.step_helpers import print_request_on_fail, render_text, step_require


@given('username {user}')
def step_login_with_username(context, user):
    """
    Login with custom username
    """
    context.username = user


@given('dataproc agent is updated to the latest version')
@then('update dataproc agent to the latest version')
def step_update_dataproc_agent(context):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    HadoopCluster(context).update_dataproc_agent()


@when('we execute "{command}" on master node')
def step_run_command_on_master_node(context, command):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    HadoopCluster(context).run_command(command)


@then('s3 file {uri} not contains')
def step_check_s3_file_contains(context, uri):
    uri = render_text(context, uri)
    if not uri.startswith("s3://"):
        raise Exception(f'Unsupported uri: {uri}')

    parsed_uri = urlparse(uri)
    bucket_name = parsed_uri.netloc
    object_name = parsed_uri.path[1:]
    file_content = s3.download_object(context.conf['s3'], bucket_name, object_name)

    assert context.text not in file_content, 'The file {uri} contains text {body}'.format(uri=uri, body=context.text)


@then('s3 file {uri} contains')
def step_check_s3_file_not_contains(context, uri):
    uri = render_text(context, uri)
    if not uri.startswith("s3://"):
        raise Exception(f'Unsupported uri: {uri}')

    parsed_uri = urlparse(uri)
    bucket_name = parsed_uri.netloc
    object_name = parsed_uri.path[1:]
    file_content = s3.download_object(context.conf['s3'], bucket_name, object_name)

    assert context.text in file_content, 'Not found text {body} within s3 file {uri}'.format(body=context.text, uri=uri)


@then('combined content of files within s3 folder {uri} contains')
def step_check_files_within_s3_folder_contain(context, uri):
    uri = render_text(context, uri)
    if not uri.startswith("s3://"):
        raise Exception(f'Unsupported uri: {uri}')

    parsed_uri = urlparse(uri)
    bucket_name = parsed_uri.netloc
    folder_name = parsed_uri.path[1:]
    combined_content = s3.combined_content(context.conf['s3'], bucket_name, folder_name)

    assert context.text in combined_content, 'Not found text {body} within files in s3 folder {uri}'.format(
        body=context.text, uri=uri
    )


@then('job driver output contains')
def job_driver_output_contains(context):
    output, error = internal_api.get_dataproc_job_log(context)
    assert error is None, f'Got error when fetched job log: {error}'
    found = context.text in output
    assert found, f'Not found text {context.text} within job driver output. Output: {output}'


@then('print job results')
def job_driver_output_print(context):
    output, error = internal_api.get_dataproc_job_log(context)
    if error:
        print(error)
    print(output)


@then('job output does not contain')
@then('job driver output does not contain')
def job_driver_output_not_contains(context):
    output, error = internal_api.get_dataproc_job_log(context)
    assert error is None, f'Got error when fetched job log: {error}'
    missing = context.text not in output
    assert missing, 'Expected job driver\'s output not to contain text {body}'.format(body=context.text)


@then('job output fails and error contains "{expected_error}"')
def job_driver_output_fails(context, expected_error):
    output, error = internal_api.get_dataproc_job_log(context)
    assert expected_error in error, f'Expected error on gathering job ouput differs: {error}'


@then('job output is empty')
@then('job driver output is empty')
def job_driver_output_is_empty(context):
    output, error = internal_api.get_dataproc_job_log(context)
    assert error is None, f'Got error when fetching job log: {error}'
    assert output == '', 'Expected job driver\'s output to be empty'


@when('we try to make "{size}" random file "{path}" in hdfs')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def create_random_hdfs_file(context, size, path):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cluster = HadoopCluster(context)
    hash_sum, _ = cluster.create_hdfs_file(path, size)
    if not hasattr(context, 'hdfs_file_hash_sum'):
        context.hdfs_file_hash_sum = dict()
    context.hdfs_file_hash_sum[path] = hash_sum
    # saving hashes for manual debugging, when context is lost
    with open(context.conf['saved_context_path'], 'wb') as outfile:
        pickle.dump(context.hdfs_file_hash_sum, outfile)


@then('hash sum of "{path}" from hdfs is ok')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def get_hdfs_file_hash_sum(context, path):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cluster = HadoopCluster(context)
    hash_sum, _ = cluster.get_hdfs_file_hash_sum(path)
    if not hasattr(context, 'hdfs_file_hash_sum'):
        # retrieving hashes if the context is lost
        context.hdfs_file_hash_sum = pickle.load(open(context.conf['saved_context_path'], 'rb'))
    assert hash_sum == context.hdfs_file_hash_sum[path]


@then('no node in cluster holds all blocks of "{path}"')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def check_several_data_nodes(context, path: str):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cluster = HadoopCluster(context)
    does_any_nodes_contain_all_blocks, stdout = cluster.check_hdfs_file_block_locations(path)
    assert not does_any_nodes_contain_all_blocks, stdout


@then('there is a node that contains all blocks of "{path}"')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def check_data_nodes_number(context, path: str):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cluster = HadoopCluster(context)
    does_any_nodes_contain_all_blocks, stdout = cluster.check_hdfs_file_block_locations(path)
    assert does_any_nodes_contain_all_blocks, stdout


@then('run ssh command "{command}" for instance group hosts')
@print_request_on_fail
def ssh_for_instance_group_hosts(context, command: str):
    if hasattr(context, 'cluster_hosts'):
        cluster = HadoopCluster(context)
        instance_group_hosts = [host_info['name'] for host_info in context.cluster_hosts if '-g-' in host_info['name']]
        cluster.ssh_for_cluster_hosts(instance_group_hosts, command)


@then('load "{number:d}" CPUs of each instance group hosts')
def load_cpus_instance_group_hosts(context, number: int):
    command = ' '.join(['cat /dev/zero > /dev/null &'] * number)
    ssh_for_instance_group_hosts(context, command)


@then('unload CPUs of instance group hosts')
def unload_cpus_instance_group_hosts(context):
    ssh_for_instance_group_hosts(context, 'pkill -9 cat || true')
