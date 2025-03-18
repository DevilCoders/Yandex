from serp import QueryKey  # noqa
import yaqutils.misc_helpers as umisc


@umisc.hash_and_ordering_from_key_method
class FieldExtenderKey(object):
    def __init__(self, query_key, url, pos):
        """
        :type query_key: QueryKey
        :type url: str
        :type pos: int | None
        """
        self.query_key = query_key
        if not umisc.is_string_or_unicode(url):
            raise Exception("Incorrect type for 'url' in FieldExtenderKey: '{}'".format(type(url)))
        self.url = url
        if not umisc.is_integer_or_none(pos):
            raise Exception("Incorrect type for 'pos' in FieldExtenderKey: '{}'".format(type(pos)))
        self.pos = pos

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "FieldExtKey({})".format(self.key())

    def key(self):
        return self.query_key.query_text, self.query_key.query_region, self.url, self.pos
