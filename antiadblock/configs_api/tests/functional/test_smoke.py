
def test_ping(app, session):
    r = session.get(app.ping)

    assert r.status_code == 200
    assert r.content == 'OK'
