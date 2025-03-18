import urllib


def test_compose_works():
    # import pdb; pdb.set_trace()
    response = urllib.urlopen("http://localhost:5000").read()

    assert 'Hello World!' in response
