# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest
from django.conf import settings
from django.db.utils import InternalError, OperationalError

from django_pgaas import atomic_retry
from .app.models import RandomModel


def patch_compiler(klass, exception=OperationalError):
    return mock.patch(
        'django.db.models.sql.compiler.%s.execute_sql' % klass,
        new=mock.Mock(side_effect=exception)
    )


@pytest.mark.django_db(transaction=True)
def test_retry_select_outside_atomic():

    with patch_compiler('SQLCompiler') as mocked:
        with pytest.raises(OperationalError):
            list(RandomModel.objects.all())

        assert mocked.call_count == (settings.PGAAS_RETRY_ATTEMPTS + 1)


@pytest.mark.django_db(transaction=True)
def test_no_retry_internal_error():

    with patch_compiler('SQLCompiler', InternalError('any error')) as mocked:
        with pytest.raises(InternalError):
            list(RandomModel.objects.all())

        assert mocked.call_count == 1


@pytest.mark.xfail
@pytest.mark.django_db(transaction=True)
def test_retry_insert_outside_atomic():

    with patch_compiler('SQLInsertCompiler') as mocked:
        with pytest.raises(OperationalError):
            RandomModel.objects.create(name='asdfasdf')

        assert mocked.call_count == (settings.PGAAS_RETRY_ATTEMPTS + 1)


@pytest.mark.xfail
@pytest.mark.django_db(transaction=True)
def test_retry_update_outside_atomic():

    with patch_compiler('SQLUpdateCompiler') as mocked:
        with pytest.raises(OperationalError):
            RandomModel.objects.update(name='asdfasdf')

        assert mocked.call_count == (settings.PGAAS_RETRY_ATTEMPTS + 1)


@pytest.mark.xfail
@pytest.mark.django_db(transaction=True)
def test_retry_delete_outside_atomic():

    with patch_compiler('SQLDeleteCompiler') as mocked:
        with pytest.raises(OperationalError):
            RandomModel.objects.all().delete()

        assert mocked.call_count == (settings.PGAAS_RETRY_ATTEMPTS + 1)


@pytest.mark.django_db(transaction=False)
def test_no_retry_inside_atomic():

    with patch_compiler('SQLCompiler') as mocked:
        with pytest.raises(OperationalError):
            RandomModel.objects.all().count()

        assert mocked.call_count == 1


@pytest.mark.django_db(transaction=True)
def test_atomic_retry():

    def do_something():
        # Sanity check for we are inside an atomic block
        RandomModel.objects.select_for_update().count()
        raise OperationalError()

    mocked = mock.Mock(side_effect=do_something)

    retriable = atomic_retry()(mocked)

    with pytest.raises(OperationalError):
        retriable()

    assert mocked.call_count == (settings.PGAAS_RETRY_ATTEMPTS + 1)
