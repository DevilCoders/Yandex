"""
Simple resource manager mock
"""

from httmock import urlmatch

from .utils import handle_http_action, http_return


def resource_manager(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='resource-manager.test', method='get', path='/folders/test_folder:listAccessBindings')
    def list_access_bindings(url, _):
        ret = handle_http_action(state, 'resource-manager-list-access-bindings')
        if ret:
            return ret

        return http_return(
            body={
                'accessBindings': [
                    {
                        'subject': {'id': 'test-service-account', 'type': 'serviceAccount'},
                        'roleId': 'dataproc.agent',
                    }
                ]
            }
        )

    return (list_access_bindings,)
