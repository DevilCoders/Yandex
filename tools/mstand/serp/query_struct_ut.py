# -*- coding: utf-8 -*-

from .query_struct import QueryKey


class TestQueryKey:
    def test_query_key(self):
        query_key = QueryKey(query_text="text", query_region=100500)
        hash(query_key)

    def test_query_key_unicode(self):
        query_key1 = QueryKey(query_text=u"Яндекс", query_region=100500)
        query_key2 = QueryKey(query_text=u"Яндекс", query_region=100500)
        assert len({query_key1, query_key2}) == 1

    def test_query_key_uid_unicode(self):
        query_key = QueryKey(query_text=u"Яндекс", query_region=100500, query_uid=u"а вот и такое бывает")
        hash(query_key)
