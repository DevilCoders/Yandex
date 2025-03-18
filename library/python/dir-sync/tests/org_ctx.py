# coding: utf-8

from django.test import TestCase

from dir_data_sync.models import Organization
from dir_data_sync.org_ctx import org_ctx, get_org


class OrgCtxTest(TestCase):
    def setUp(self):
        super(OrgCtxTest, self).setUp()
        org = Organization(name='7', dir_id='7', label='7')
        org.save()
        self.org_7 = org
        org = Organization(name='42', dir_id='42', label='42')
        org.save()
        self.org_42 = org

    def test_empty_org_ctx(self):
        self.assertIsNone(get_org())

    def test_org_ctx_nesting(self):
        # Проверяем вложенность контекста организации.
        self.assertIsNone(get_org())
        with org_ctx(self.org_7):
            self.assertEquals(self.org_7, get_org())
            with org_ctx(self.org_42):
                self.assertEquals(self.org_42, get_org())
            self.assertEquals(self.org_7, get_org())
        self.assertIsNone(get_org())

    def test_org_ctx_dropped_after_exception(self):
        # Проверяем, что возникновении исключения внутри блока
        # с контекстом организации, при выходе из блока контекст сбрасывается.
        self.assertIsNone(get_org())
        try:
            with org_ctx(self.org_7):
                with org_ctx(self.org_42):
                    raise ValueError
        except ValueError:
            self.assertIsNone(get_org())
