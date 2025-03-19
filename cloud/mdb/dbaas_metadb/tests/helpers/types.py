"""
Types
"""

from behave import register_type
from parse import with_pattern
from parse_type import TypeBuilder
from psycopg2.extensions import ISQLQuote, adapt
from psycopg2.extras import register_uuid

register_uuid()


class Enum:  # pylint:disable=too-few-public-methods
    """
    Simple enum adapter
    """

    def __init__(self, db_type, value):
        self.db_type = db_type
        self.value = value

    def __conform__(self, proto):
        if proto is ISQLQuote:
            return self
        return None

    def getquoted(self):
        """
        Quoted representaion
        """
        return b'%s::%s' % (adapt(self.value).getquoted(), self.db_type.encode('ascii'))


@with_pattern(r'[\w.-]+')
def parse_word_with_dots(text):
    """
    Foo.Bar-Baz type
    """
    return text


register_type(
    Word=parse_word_with_dots,
    PillarKey=TypeBuilder.make_enum(
        {
            'host': 'fqdn',
            'cluster': 'cid',
            'subcluster': 'subcid',
        }
    ),
)
