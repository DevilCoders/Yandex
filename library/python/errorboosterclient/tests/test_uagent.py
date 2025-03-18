from os import environ


def test_integration():

    from errorboosterclient.uagent import UnifiedAgentSender

    environ['ERROR_BOOSTER_HTTP_HOST'] = 'myhost'
    environ['ERROR_BOOSTER_HTTP_PORT'] = '0000'

    data_log = []

    def mock_request(body):
        data_log.append(body)

    sender = UnifiedAgentSender()
    sender.request = mock_request

    sender({'one': 'two'})

    assert data_log == ['{"one": "two"}']
