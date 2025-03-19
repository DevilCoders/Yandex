from unittest.mock import Mock


def kubernetes_master(mocker, state):
    mocker.patch('cloud.mdb.dbaas_worker.internal.providers.kubernetes_master.client.ApiClient').return_value = Mock()

    batch_api = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.kubernetes_master.client.BatchV1Api'
    ).return_value
    response = Mock()
    response.status.succeeded = True
    response.status.failed = False
    batch_api.read_namespaced_job_status.return_value = response

    core_api = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.kubernetes_master.client.CoreV1Api').return_value
    response = Mock()
    endpoint = Mock()
    port = Mock()
    port.node_port = 42
    endpoint.ip = '222.222.222.22'
    response.status.load_balancer.ingress = [endpoint]
    response.spec.ports = [port]
    core_api.read_namespaced_service.return_value = response

    mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.kubernetes_master.client.CustomObjectsApi'
    ).return_value = Mock()
