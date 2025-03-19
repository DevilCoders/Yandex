"""
Initialize certificate generating class
"""
from . import ca, utils


@utils.env_stage('create', fail=True)
def init_ssl(state=None, **_):
    """
    Initialize SSL certificates.
    """
    assert state is not None, 'state must not be None'
    # Clean slate
    ssl_obj = state.get('ssl')
    if not isinstance(ssl_obj, ca.PKI):
        state['ssl'] = ca.PKI()
        return


@utils.env_stage('create', fail=True)
def dump_ssl(state=None, **_):
    """
    Dump certs for fake_certificator.
    """
    assert state is not None, 'state must not be None'
    ssl_obj = state['ssl']
    assert isinstance(ssl_obj, ca.PKI), 'ssl in state is incorrect'
    state['ssl'].dump()
