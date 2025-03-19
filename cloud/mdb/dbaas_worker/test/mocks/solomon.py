"""
Simple solomon config mock
"""


from httmock import urlmatch

from .utils import handle_http_action, http_return


def alert_ext_id():
    i = 0
    while True:
        i += 1
        yield str(i)


def solomon_api_called(state: dict):
    state['solomon']['calls_to_solomon_api'] += 1


def solomon(state):
    """
    Setup mock with state
    """
    seq_alert_ext_id = alert_ext_id()

    @urlmatch(netloc='solomon.test', method='get')
    def read_handle(url, _):
        solomon_api_called(state)
        ret = handle_http_action(state, f'solomon-alert.get-{url.path}')
        if ret:
            return ret

        return http_return(code=404, body={'code': 404})

    @urlmatch(netloc='solomon.test', method='post')
    def create_alert_handle(url, _):
        solomon_api_called(state)
        ret = handle_http_action(state, f'solomon-alert.post.{url.path}')
        if ret:
            return ret

        container = state['solomon']['alerts']

        id = next(seq_alert_ext_id)

        container.append(
            {
                'alert_ext_id': id,
            }
        )

        return http_return(
            body={
                'id': id,
            },
        )

    @urlmatch(netloc='solomon.test', method='delete')
    def delete_alert_handle(url, request):
        solomon_api_called(state)
        ret = handle_http_action(state, f'solomon-alert.delete-{url.path}')
        if ret:
            return ret

        alert_id = request.path_url.split('/')[-1]

        if alert_id == 'test-deleting-id':
            return http_return(
                body={},
            )
        index = -1
        container = state['solomon']['alerts']
        for index, internal_alert in enumerate(container):
            if internal_alert['alert_ext_id'] == alert_id:
                break
        if index != -1:
            container.pop(index)
            return http_return(code=200, body={})
        else:
            return http_return(code=404, body={'code': 404})

    return (
        read_handle,
        create_alert_handle,
        delete_alert_handle,
    )
