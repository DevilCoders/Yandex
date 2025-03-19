import os


def pytest_configure():
    os.environ['APP_DIR'] = os.path.join(
        os.path.dirname(__file__),
        '..',
    )
    os.environ['DJANGO_SETTINGS_MODULE'] = 'cloud.mdb.backstage.settings.settings'

    import django
    django.is_in_test = True
    django.setup()


def pytest_unconfigure():
    import django
    del django.is_in_test
