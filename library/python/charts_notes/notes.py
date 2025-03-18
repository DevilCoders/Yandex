from library.python.charts_notes.utils import remove_nulls


class Note(object):
    """ Base class for notes """
    def __init__(self, text, color=None):
        self.text = text
        self.color = color

    def to_json(self):
        raise NotImplementedError('Unable convert to json base `Note` class')


class DashStyle(object):
    """
    Enum for dash style
    See http://jsfiddle.net/es6ejm5v/embedded/result/
    """
    SOLID = 'solid'
    SHORT_DASH = 'shortdash'
    SHORT_DOT = 'shortdot'
    SHORT_DASH_DOT = 'shortdashdot'
    SHORT_DASH_DOT_DOT = 'shortdashdotdot'
    DOT = 'dot'
    DASH = 'dash'
    LONG_DASH = 'longdash'
    DASH_DOT = 'dashdot'
    LONG_DASH_DOT = 'longdashdot'
    LONG_DASH_DOT_DOT = 'longdashdotdot'


class FlagShape(object):
    """ Enum for flag type """
    CIRCLEPIN = 'circlepin'
    SQUAREPIN = 'squarepin'
    FLAG = 'flag'


class Line(Note):
    NOTE_TYPE = 'line-x'

    def __init__(self, text, color=None, dash_style=DashStyle.SOLID, width=2):
        super(Line, self).__init__(text, color=color)
        self.dash_style = dash_style
        self.width = width

    def to_json(self):
        return remove_nulls({
            'type': self.NOTE_TYPE,
            'text': self.text,
            'meta': {
                'color': self.color,
                'dashStyle': self.dash_style,
                'width': self.width,
            }
        })

    @staticmethod
    def from_json(data):
        return Line(
            text=data['text'],
            color=data.get('meta', {}).get('color'),
            dash_style=data.get('meta', {}).get('dashStyle'),
            width=data.get('meta', {}).get('width'),
        )


class Band(Note):
    NOTE_TYPE = 'band-x'

    def __init__(self, text, color=None, visible=False, z_index=0):
        super(Band, self).__init__(text, color=color)
        self.visible = visible
        self.z_index = z_index

    def to_json(self):
        return remove_nulls({
            'type': self.NOTE_TYPE,
            'text': self.text,
            'meta': {
                'color': self.color,
                'visible': self.visible,
                'zIndex': self.z_index,
            }
        })

    @staticmethod
    def from_json(data):
        return Band(
            text=data['text'],
            color=data.get('meta', {}).get('color'),
            visible=data.get('meta', {}).get('visible', False),
            z_index=data.get('meta', {}).get('zIndex'),
        )


class Flag(Note):
    NOTE_TYPE = 'flag-x'

    def __init__(self, text, color=None, y=-30, shape=FlagShape.CIRCLEPIN):
        super(Flag, self).__init__(text, color=color)
        self.y = y
        self.shape = shape

    def to_json(self):
        return remove_nulls({
            'type': self.NOTE_TYPE,
            'text': self.text,
            'meta': {
                'color': self.color,
                'y': self.y,
                'shape': self.shape,
            }
        })

    @staticmethod
    def from_json(data):
        return Flag(
            text=data['text'],
            color=data.get('meta', {}).get('color'),
            y=data.get('meta', {}).get('y'),
            shape=data.get('meta', {}).get('shape'),
        )


class Dot(Note):
    NOTE_TYPE = 'dot-x-y'

    def __init__(self, text, graph_id, color=None, visible=False, fill_color=None, text_color=None):
        super(Dot, self).__init__(text, color=color)
        self.graph_id = graph_id
        self.visible = visible
        self.fill_color = fill_color
        self.text_color = text_color

    def to_json(self):
        return remove_nulls({
            'type': self.NOTE_TYPE,
            'text': self.text,
            'meta': {
                'color': self.color,
                'graphId': self.graph_id,
                'visible': self.visible,
                'fillColor': self.fill_color,
                'textColor': self.text_color,
            }
        })

    @staticmethod
    def from_json(data):
        return Dot(
            text=data['text'],
            graph_id=data.get('meta', {})['graphId'],
            color=data.get('meta', {}).get('color'),
            visible=data.get('meta', {}).get('visible', False),
            fill_color=data.get('meta', {}).get('fillColor'),
            text_color=data.get('meta', {}).get('textColor'),
        )


_ALL_NOTES = [Line, Band, Flag, Dot]
_NOTES_TYPES = {n.NOTE_TYPE: n for n in _ALL_NOTES}


def note_from_json(data):
    note_type = data['type']
    if note_type not in _NOTES_TYPES:
        raise ValueError('Unknown note type: {}'.format(note_type))

    return _NOTES_TYPES[note_type].from_json(data)
