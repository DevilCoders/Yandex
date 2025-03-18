import re
from base64 import b64encode
from functools import reduce
from json import dumps
from operator import or_
from typing import Dict, Tuple, Union, Any, Iterable, Optional, List

from .utils import file_read

TypeColorRgb = Tuple[int, int, int]
TypeTextCompat = Union[str, Iterable[str], 'Text', Iterable['Text']]
TypeStampCompat = Union[str, Iterable[str], 'Stamp']

COLOR_GRAY = (128, 128, 128)
COLOR_WHITE = (255, 255, 255)
COLOR_BLACK = (0, 0, 0)
COLOR_RED = (255, 0, 0)
COLOR_GREEN = (0, 128, 0)
COLOR_BLUE = (0, 0, 255)
COLOR_MAROON = (128, 0, 0)
COLOR_NAVY = (0, 0, 128)

COLORS: Dict[str, TypeColorRgb] = {
    'gray': COLOR_GRAY,
    'white': COLOR_WHITE,
    'black': COLOR_BLACK,
    'red': COLOR_RED,
    'green': COLOR_GREEN,
    'blue': COLOR_BLUE,
    'maroon': COLOR_MAROON,
    'navy': COLOR_NAVY,
}


RE_FORMAT_MARKERS = re.compile('^(!(?P<markers>[^!]+)!)?(?P<text>.+)')


def to_rgb(color: Union[str, TypeColorRgb]) -> Dict:
    """Возвращает словарь с RGB нотацией цвета, созданный по данным строки или кортежа.

    :param color:

    """
    if isinstance(color, str):
        color = COLORS[color]

    result = {
        'Red': color[0],
        'Green': color[1],
        'Blue': color[2],
    }

    return result


class Box:
    """Бокс (прямоугольная область) для печати."""

    def __init__(self, bg_color: Union[str, TypeColorRgb] = None, margin: int = None):
        """
        :param bg_color: Цвет фона.
            Кортеж RGB (0, 0, 0), либо название цвета, например: white, black
            По умолчанию: без заливки.

        :param margin: Отступ до содержимого со всех сторон.

        """
        self._border = None
        self.bg = bg_color
        self.margin = margin or 5

        self.width = 0
        self.height = 0
        self.pos_x = 0
        self.pos_y = 0

        self.set_border()
        self.set_size()
        self.set_position()

    def set_size(self, width: int = None, height: int = None):
        """Устанавливает размеры области.

        :param width: Ширина.
        :param height: Высота.

        """
        self.width = width or 200
        self.height = height or 80

    def set_position(self, x: int = None, y: int = None):
        """Устанавливает место расположения области на листе.

        ВНИМАНИЕ: Принимает координаты левого нижнего угла области, относительно НИЗА страницы.

        :param x:
        :param y:

        """
        self.pos_x = x or 20
        self.pos_y = y or 20

    def set_border(self, width: int = None, radius: int = None, color: Union[str, TypeColorRgb] = None):
        """Устаналивает параметры границ бокса.

        :param width: Толщина.

        :param radius: Радиус закругления.

        :param  color: Цвет.
                Кортеж RGB (0, 0, 0), либо название цвета, например: white, black

        """
        self._border = {
            'BorderRadius': radius or 0,
            'BorderWeight': width or 1,
            'BorderColor': to_rgb(color or COLOR_BLACK),
        }

    def asdict(self) -> dict:
        pos_x = self.pos_x
        pos_y = self.pos_y

        result = {
            'LowerLeftX': pos_x,
            'LowerLeftY': pos_y,
            'UpperRightX': pos_x + self.width,
            'UpperRightY': pos_y + self.height,
            'ContentMargin': self.margin,
        }
        if self.bg:
            result['BackgroundColor'] = to_rgb(self.bg)

        result.update(self._border)

        return result


class Font:
    """Шрифт."""

    STYLE_NONE = 0
    STYLE_BOLD = 1
    STYLE_ITALIC = 2
    STYLE_UNDERLINE = 4
    STYLE_STRIKE = 8

    _style_map = {
        'b': STYLE_BOLD,
        'i': STYLE_ITALIC,
        'u': STYLE_UNDERLINE,
        's': STYLE_STRIKE,
    }

    def __init__(self, size: int = None, family: str = None, color: Union[str, TypeColorRgb] = None, style: int = None):
        """
        :param size: Размер. По умолчанию: 8.

        :param family: Семейство. По умолчанию: 'times'
            Возможные значения: times, arial.

        :param color: Цвет в нотации RGB. По умолчанию: чёрный.
                Кортеж RGB (0, 0, 0), либо название цвета, например: white, black

        :param style: Стиль (начертание). См. STYLE_*.
            Стили сочетаются при помощи |
            Например: STYLE_ITALIC | STYLE_STRIKE

        """
        self.size = size or 8
        self.family = family or 'times'
        self.color = color or COLOR_BLACK
        self.style = style or self.STYLE_NONE

    def apply_markers(self, marker_map: dict):
        """Применяет к данном объекту указанные маркеры форматирования.

        :param marker_map:

        """
        if not marker_map:
            return

        size = marker_map.get('S')
        if size:
            self.size = int(size)

        family = marker_map.get('f')
        if family:
            self.family = family

        color = marker_map.get('c')
        if color in COLORS.keys():
            self.color = color

        style = marker_map.get('s', '')
        if style:
            self.style = reduce(
                or_, [self._style_map[letter] for letter in style if letter in self._style_map],
                self.STYLE_NONE)

    def asdict(self) -> dict:
        result = {
            'FontSize': self.size,
            'FontFamily': self.family,
            'FontStyle': self.style,
            'FontColor': to_rgb(self.color)
        }
        return result


class Text:
    """Текст"""

    def __init__(self, value: str, font: Font = None, margin_left: int = None):
        """
        :param value:

        :param font: Шрифт.

        :param margin_left: Отступ слева [от границы бокса].

        """
        self.value = value
        self.font = font or Font()
        self.margin_left = margin_left

    def apply_markers(self, marker_map: dict):
        """Применяет к данном объекту указанные маркеры форматирования.

        :param marker_map:

        """
        if not marker_map:
            return

        margin = marker_map.get('m')
        if margin is not None:
            self.margin_left = int(margin)

        self.font.apply_markers(marker_map)

    @classmethod
    def from_object(cls, text: Any) -> 'Text':
        """Альтернативный конструктор, создающий объект текста
        из друго объекта, используя его текстовое представление.

        :param text:

        """
        text = f'{text}'
        markers = ''
        marker_map = {}

        if text:
            matched = RE_FORMAT_MARKERS.match(text)
            text = matched.group('text') or ''
            markers = matched.group('markers') or ''

        spawned = cls(text)

        for marker_line in markers.split(';'):
            marker_line = marker_line.strip()
            if not marker_line:
                continue
            marker, value = marker_line.split(':', 1)
            marker_map[marker] = value

        spawned.apply_markers(marker_map)

        return spawned

    def asdict(self) -> dict:
        result = {
            'Text': self.value,
            'Font': self.font.asdict(),
        }

        margin = self.margin_left
        if margin is not None:
            result['Margin'] = margin

        return result


class Image:
    """Изображение для помещения в печать в качестве подложки, либо заполнения штампа.

    Поддерживаемые форматы: JPEG, JPEG2000, GIF, PNG, BMP, WMF, TIFF, CCITT, JBIG2.

    """
    def __init__(self, content: Union[bytes, str], pos: Tuple[int, int] = None, scale: int = None):
        """
        :param content: Изображение в виде байт, либо путь до файла с ним.

        :param pos: Для логотипа. Координаты левого нижнего угла: (X, Y)
            По умолчанию 0,0.

        :param scale: Для логотипа. Масштаб.
            По умолчанию 100.

        """
        self.content = content
        self.pos = pos
        self.scale = scale

    def asdict(self) -> dict:
        content = self.content

        if not isinstance(content, bytes):
            content = file_read(content)

        result = {
            'Image': b64encode(content),
        }

        pos = self.pos
        if pos:
            result['LowerLeftX'] = pos[0]
            result['LowerLeftY'] = pos[1]

        scale = self.scale
        if scale is not None:
            result['Scale'] = scale

        return result


class Stamp:
    """Визуально различимая в документе печать (штамп)."""

    TEMPLATE_SIMPLE = 1
    TEMPLATE_IMAGE_BG = 2
    TEMPLATE_IMAGE = 3

    colors = COLORS
    """Цвета, для которых определены псевдонимы."""

    cls_text = Text
    cls_font = Font
    cls_box = Box
    cls_image = Image

    def __init__(self, text: TypeTextCompat = None, box: Box = None, page: int = None):
        """
        :param text: Текст для размещения в печати.

            Мини-язык форматирования для текстов печатей

            Строка должна начинаться с ! (воскл. знака), далее должны следовать пары маркер_форматирования:значение,
            разделённые ; (точка с запятой), заканчиваться инструкции форматирования должны на !.

            Доступные маркеры и значения:

                * s - стиль шрифта; значения: i - курсив, b - полужирный, u - подрчёркнутый, s - перечёркнутый);
                * S - размер шрифта, целое;
                * f - семейство шрифта; значение: arial, times;
                * c - цвет шрифта, строка; примеры значений: black, red, blue, green, gray и пр.
                * m - отступ текста, целое.

        :param box: Область, которую нужно использовать. Если не задана, создастся автоматически.

        :param int page: Номер страницы, на которой требуется разместить печать.
            Нумерация с единицы. По умолчанию: первая страница.

        """
        self._template_id = self.TEMPLATE_SIMPLE
        self._image: Optional[Image] = None
        self.content: List[Text] = []
        self.page = page
        self.box = box or Box()

        text and self.add_text(text)

    def set_background_image(self, image: Union[bytes, Image], pos: Tuple[int, int] = None, scale: int = None):
        """Устанавливает указанное изображение подложкой штампа.

        :param image: Объект изображения, либо его байты.

        :param pos: Координаты левого нижнего угла: (X, Y)

        :param scale: Масштаб.

        """
        self._template_id = self.TEMPLATE_IMAGE_BG

        if not isinstance(image, Image):
            image = self.cls_image(image, pos=pos, scale=scale)

        if pos is not None:
            image.pos = pos

        if scale is not None:
            image.scale = scale

        self._image = image

    def set_foreground_image(self, image: Union[bytes, Image]):
        """Устанавливает указанное изображение заполнением штампа, при этом текст игонируется.

        :param image: Объект изображения, либо его байты.

        """
        self._template_id = self.TEMPLATE_IMAGE
        if not isinstance(image, Image):
            image = self.cls_image(image)
        self._image = image

    def add_text(self, text: TypeTextCompat):
        """Добавляет текстовый блок в печать.

        :param  text: Строка, полноценный объект Text, либо список.

        """
        if not isinstance(text, list):
            text = [text]

        for chunk in text:

            if not isinstance(chunk, self.cls_text):
                chunk = self.cls_text.from_object(chunk)

            self.content.append(chunk)

    def asdict(self) -> dict:
        template_id = self._template_id

        result = {
            'Content': [item.asdict() for item in self.content],
            'TemplateId': template_id,
            'Rect': self.box.asdict(),
        }

        page = self.page
        if page is not None:
            result['Page'] = page

        if template_id == self.TEMPLATE_IMAGE:
            result['Background'] = self._image.asdict()

        elif template_id == self.TEMPLATE_IMAGE_BG:
            result['Icon'] = self._image.asdict()

        return result

    def serialize(self) -> bytes:
        return b64encode(dumps(self.asdict()).encode('utf-8'))
