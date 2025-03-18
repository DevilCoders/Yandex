def test_multi_app():
    from .app1.settings import NAME as NAME1
    from .app2.settings import NAME as NAME2
    assert NAME1=='app1'
    assert NAME2=='app2'
