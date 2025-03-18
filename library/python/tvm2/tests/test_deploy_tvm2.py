# coding: utf-8

from __future__ import unicode_literals

import os
from tvmauth import BlackboxTvmId as BlackboxClientId

from tvm2 import TVM2Deploy


def test_tvm2_deploy_inititialized_correctly():

    TVM2Deploy._instance = None
    os.environ['DEPLOY_BOX_ID'] = 'backend'
    os.environ['DEPLOY_TVM_TOOL_URL'] = 'http://localhost:100500'
    os.environ['TVMTOOL_LOCAL_AUTHTOKEN'] = 'deploy_test_token'

    tvm2_client = TVM2Deploy(
        client_id='28',
        blackbox_client=BlackboxClientId.Test,
        retries=0,
    )

    assert tvm2_client.api_url == 'http://localhost:100500/tvm'
    assert tvm2_client.session.headers['Authorization'] == 'deploy_test_token'

    os.environ.pop('DEPLOY_BOX_ID')
    os.environ.pop('DEPLOY_TVM_TOOL_URL')
    os.environ.pop('TVMTOOL_LOCAL_AUTHTOKEN')
