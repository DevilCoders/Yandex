from dssclient.resources.document import Document


def test_signing(fake_dss, read_fixture):

    signed1 = '9L5TbUCppln2NeOXJOR8KQpriXkCBtzRd68/FnfznpOgEx92+btUz7d1UxO4q4flHF6nJWAvnyZI31bQim82Jw=='
    dss = fake_dss(signed1)

    cert = dss.certificates.get(2, refresh=False)
    assert not dss.documents.sign([], params=dss.signing_params.ghost3410(cert))  # not docs

    cert = dss.certificates.get(2, refresh=False)
    signing_results = dss.documents.sign('signthis', params=dss.signing_params.ghost3410(cert))

    assert len(signing_results) == 1
    assert isinstance(signing_results[0], Document)
    assert signing_results[0].signed_base64 == signed1

    dss.set_response(read_fixture('signatures_ok.json'))
    signing_results = dss.documents.sign(['signthis', 'signthat'], params=dss.signing_params.ghost3410(cert))

    assert len(signing_results) == 2
    assert isinstance(signing_results[0], Document)
    assert isinstance(signing_results[1], Document)

    assert (signing_results[0].signed_base64 ==
            'yWoJVvGnv0UB7492KaF+IyY7XJtAM9Ry2kHUo88HnP8speP3Vg+PhJiKQ4c0XnBFWuc+/Q7j4694mRqfYICqTA==')
    assert (signing_results[1].signed_base64 ==
            'CUm4iYHfTbuW1Eb6VLyIkDd7I33QB7BW69oZSw7FIJui32GZsIo5zELO8c4qQsp94j+7nE5OFT7AxC50vqcKtw==')
