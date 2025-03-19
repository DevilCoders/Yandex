import typing
import unittest
from dataclasses import dataclass
from datetime import datetime, timezone, timedelta
from enum import Enum

from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable


class TestYsonSerializable(unittest.TestCase):
    def test_complex_object(self):
        class Color(Enum):
            BLUE = 'blue'
            RED = 'red'

        @dataclass
        class Flag(YsonSerializable):
            color: Color
            width: int
            height: int
            density: typing.Optional[float]

        class Material(Enum):
            WOOD = 1
            BRICK = 2

        @dataclass
        class Basement(YsonSerializable):
            depth: int

        @dataclass
        class Building(YsonSerializable):
            name: str
            flags: typing.List[Flag]
            owners: typing.List[str]
            materials: typing.List[Material]
            basement: typing.Optional[Basement]
            weight: float
            ready: bool
            checksum: bytes
            other: dict
            built_at: typing.Optional[datetime]
            construction_duration: typing.Optional[timedelta]

        yson = {
            'name': 'castle',
            'flags': [
                {
                    'color': 'red',
                    'width': 30,
                    'height': 20,
                    'density': 1.23,
                },
                {
                    'color': 'blue',
                    'width': 60,
                    'height': 40,
                    'density': None,
                },
            ],
            'owners': ['Jack', 'Bob', 'Esmeralda'],
            'materials': [2],
            'basement': {
                'depth': 40,
            },
            'weight': 123.456,
            'ready': True,
            'checksum': b'123',
            'other': {
                'foo': False,
                'bar': [1, 'gleb', True],
                'baz': {
                    1: b'abc',
                    'qwe': 3.2,
                }
            },
            'built_at': '1999-12-31T00:00:00+00:00',
            'construction_duration': 'P26DT20H12M10S',
        }
        obj = Building(
            name='castle',
            flags=[
                Flag(
                    color=Color.RED,
                    width=30,
                    height=20,
                    density=1.23,
                ),
                Flag(
                    color=Color.BLUE,
                    width=60,
                    height=40,
                    density=None,
                ),
            ],
            owners=['Jack', 'Bob', 'Esmeralda'],
            materials=[Material.BRICK],
            basement=Basement(
                depth=40,
            ),
            weight=123.456,
            ready=True,
            checksum=b'123',
            other={
                'foo': False,
                'bar': [1, 'gleb', True],
                'baz': {
                    1: b'abc',
                    'qwe': 3.2,
                }
            },
            built_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            construction_duration=timedelta(days=26, hours=20, minutes=12, seconds=10),
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, Building.from_yson(yson))

    def test_custom_serialization(self):
        class AuthorType(Enum):
            HUMAN = 'human'
            MACHINE = 'machine'

        @dataclass
        class Human(YsonSerializable):
            name: str
            age: int

            @staticmethod
            def serialization_additional_fields():
                return {'type': AuthorType.HUMAN.value}

        @dataclass
        class Machine(YsonSerializable):
            serial: str

            @staticmethod
            def serialization_additional_fields():
                return {'type': AuthorType.MACHINE.value}

        def deserialize_author(fields):
            type = fields['type']
            if type == AuthorType.HUMAN.value:
                return Human.from_yson(fields)
            elif type == AuthorType.MACHINE.value:
                return Machine.from_yson(fields)
            else:
                raise ValueError(f'Unexpected author type: {type}')

        @dataclass
        class Authorship(YsonSerializable):
            license: str
            author: typing.Union[Human, Machine]

            @staticmethod
            def deserialization_overrides():
                return {
                    'author': deserialize_author,
                }

        class FigureType(Enum):
            CIRCLE = 'circle'
            RECTANGLE = 'rectangle'

        @dataclass
        class Circle(YsonSerializable):
            radius: int

            @staticmethod
            def serialization_additional_fields():
                return {'type': FigureType.CIRCLE.value}

        @dataclass
        class Rectangle(YsonSerializable):
            width: int
            height: int

            @staticmethod
            def serialization_additional_fields():
                return {'type': FigureType.RECTANGLE.value}

        def deserialize_figure(fields):
            type = fields['type']
            if type == FigureType.CIRCLE.value:
                return Circle.from_yson(fields)
            elif type == FigureType.RECTANGLE.value:
                return Rectangle.from_yson(fields)
            else:
                raise ValueError(f'Unexpected figure type: {type}')

        @dataclass
        class Picture(YsonSerializable):
            name: str
            main_figure: typing.Union[Circle, Rectangle]
            other_figures: typing.List[typing.Union[Circle, Rectangle]]
            authorship: Authorship

            @staticmethod
            def deserialization_overrides():
                return {
                    'main_figure': deserialize_figure,
                    'other_figures': deserialize_figure,
                }

        yson = {
            'name': 'wow',
            'main_figure': {
                'radius': 7,
                'type': 'circle'
            },
            'other_figures': [
                {
                    'width': 10,
                    'height': 6,
                    'type': 'rectangle'
                },
                {
                    'radius': 3,
                    'type': 'circle'
                },
            ],
            'authorship': {
                'license': 'free',
                'author': {
                    'name': 'Bob',
                    'age': 42,
                    'type': 'human',
                },
            },
        }
        obj = Picture(
            name='wow',
            main_figure=Circle(
                radius=7,
            ),
            other_figures=[
                Rectangle(
                    width=10,
                    height=6,
                ),
                Circle(
                    radius=3,
                ),
            ],
            authorship=Authorship(
                license='free',
                author=Human(
                    name='Bob',
                    age=42,
                ),
            ),
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, Picture.from_yson(yson))

    def test_nones(self):
        @dataclass
        class Foo(YsonSerializable):
            name: str

        @dataclass
        class Bar(YsonSerializable):
            foo_value: typing.Optional[Foo]
            foo_list: typing.List[typing.Optional[Foo]]
            number: typing.Optional[int]
            date: typing.Optional[datetime]

        yson = {
            'foo_value': None,
            'foo_list': [None],
            'number': None,
            'date': None,
        }
        obj = Bar(
            foo_value=None,
            foo_list=[None],
            number=None,
            date=None,
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, Bar.from_yson(yson))

        yson = {
            'foo_value': {'name': 'qqq'},
            'foo_list': [None, {'name': 'abc'}],
            'number': 42,
            'date': '1999-12-31T00:00:00+00:00',
        }
        obj = Bar(
            foo_value=Foo(name='qqq'),
            foo_list=[None, Foo(name='abc')],
            number=42,
            date=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, Bar.from_yson(yson))

    def test_not_supported(self):
        @dataclass
        class FlagNonSerializable:
            foo: int

        @dataclass
        class BuildingNonSerializable1(YsonSerializable):
            bar: str
            flag: FlagNonSerializable

        with self.assertRaises(ValueError):
            BuildingNonSerializable1(
                bar='baz',
                flag=FlagNonSerializable(
                    foo=42,
                ),
            ).to_yson()

    def test_serialization_only_annotated_members(self):
        @dataclass
        class Foo(YsonSerializable):
            bar: int

        foo = Foo(bar=42)
        foo.unknown = 'abc'

        self.assertEqual({'bar': 42}, foo.to_yson())

    def test_deserialization_with_unannotated_field(self):
        @dataclass
        class Foo(YsonSerializable):
            bar: int

        self.assertEqual(Foo(bar=42), Foo.from_yson({'bar': 42, 'unkwnown': 'abc'}))

    def test_custom_attrs_not_suppressed(self):
        @dataclass
        class BuildingWithMethod(YsonSerializable):
            name: str

            def join_names(self, other: 'BuildingWithMethod') -> str:
                return ' '.join((self.name, other.name))

        self.assertEqual({'name': 'castle'}, BuildingWithMethod(name='castle').to_yson())
        self.assertEqual('castle barn', BuildingWithMethod(name='castle').join_names(BuildingWithMethod(name='barn')))

    def test_class_without_annotations(self):
        class Foo(YsonSerializable):
            def __init__(self, bar: int):
                self.bar = bar

        with self.assertRaises(AttributeError):
            Foo(bar=42).to_yson()

    def test_hierarchy(self):
        @dataclass
        class Foo(YsonSerializable):
            a: int
            b: bool

        @dataclass
        class Bar(Foo):
            b: typing.List[str]
            c: bool

        obj = Bar(a=42, b=['abc', 'qqq'], c=True)
        yson = {
            'a': 42,
            'b': ['abc', 'qqq'],
            'c': True,
        }

        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, Bar.from_yson(yson))

    def test_override_from_yson(self):
        @dataclass
        class Foo(YsonSerializable):
            a: str
            b: typing.Optional[int]

            @classmethod
            def from_yson(cls, fields: dict):
                return Foo(
                    a=fields['a'],
                    b=fields.get('b'),
                )

        @dataclass
        class Bar(YsonSerializable):
            name: str
            foo: Foo

        self.assertEqual(Bar(
            name='abc',
            foo=Foo(
                a='qwe',
                b=None,
            ),
        ), Bar.from_yson({
            'name': 'abc',
            'foo': {
                'a': 'qwe',
            },
        }))

    def test_primary_key_sort(self):
        class Color(Enum):
            RED = 'red'
            BLUE = 'blue'

            def __lt__(self, other):
                return self.value < other.value

        @dataclass
        class Foo(OrderedYsonSerializable):
            field3: Color
            field2: int
            field1: str
            comment: str

            @staticmethod
            def primary_key() -> typing.List[str]:
                return ['field1', 'field2', 'field3']

        self.assertEqual(
            [
                Foo(field1='aaa', field2=7, field3=Color.RED, comment='object 5'),
                Foo(field1='aaa', field2=8, field3=Color.BLUE, comment='object 7'),
                Foo(field1='ttt', field2=42, field3=Color.RED, comment='object 1'),
                Foo(field1='ttt', field2=42, field3=Color.RED, comment='object 6'),
                Foo(field1='zzz', field2=7, field3=Color.BLUE, comment='object 4'),
                Foo(field1='zzz', field2=7, field3=Color.RED, comment='object 3'),
                Foo(field1='zzz', field2=13, field3=Color.BLUE, comment='object 2'),
            ],
            sorted([
                Foo(field1='ttt', field2=42, field3=Color.RED, comment='object 1'),
                Foo(field1='zzz', field2=13, field3=Color.BLUE, comment='object 2'),
                Foo(field1='zzz', field2=7, field3=Color.RED, comment='object 3'),
                Foo(field1='zzz', field2=7, field3=Color.BLUE, comment='object 4'),
                Foo(field1='aaa', field2=7, field3=Color.RED, comment='object 5'),
                Foo(field1='ttt', field2=42, field3=Color.RED, comment='object 6'),
                Foo(field1='aaa', field2=8, field3=Color.BLUE, comment='object 7'),
            ])
        )
