import traceback
import copy
import math

"""
Some output controls. Rendered on frontend side.
"""

"""
Data convertors. Used to round floats (etc)
"""


class Converters(object):
    @staticmethod
    def percents(v):
        return str('{0:.2f}%'.format(v))

    @staticmethod
    def sign_percents(v):
        return str('{0:+.2f}%'.format(v))

    @staticmethod
    def smart_float(v):
        if v > 0:
            sign = ''
        else:
            v = -v
            sign = '-'

        if v < 100:
            return sign + str('{0:.2f}'.format(v))
        else:
            v = int(v)
            result = []
            while v > 0:
                if v < 1000:
                    result.append("{0:d}".format(v))
                else:
                    result.append("{0:03d}".format(v % 1000))
                v /= 1000
            return sign + " ".join(reversed(result))


"""
Front table column. Every column consists of 3 entities:
 - headear : first <th> row (simple description of table column)
 - filter : second <th> row (clickable entity used to filter table rows)
 - data : all <td> rows
"""


class TTableColumn(object):
    def __init__(self, descr, extended_descr, tp, cells_data, css='', converter=None, flt_form_name=None,
                 fltmode="fltdisabled", width="auto", flt_start_value=None):
        if descr == "Unused hosts" and extended_descr == "host":
            raise Exception("OOPS")

        self.descr = descr
        self.extended_descr = extended_descr
        self.tp = tp
        self.cells_data = cells_data
        self.css = css

        self.converter = converter
        self.width = width

        self.flt_form_name = flt_form_name  # we need this to restore filter values
        self.fltmode = fltmode
        if self.fltmode == "fltchecked":
            self.flt_start_value = int(flt_start_value) if flt_start_value else 0
        elif self.fltmode == "fltregex":
            self.flt_start_value = flt_start_value if flt_start_value else ""
        elif self.fltmode == "fltrange":
            self.flt_start_value = flt_start_value if flt_start_value else ""

    def gen_header_cell(self):
        result = {
            "descr": self.descr,
            "help": self.extended_descr,
            "width": self.width,
        }

        return result

    def gen_filter_cell(self):
        if self.fltmode == "fltchecked":
            result = {
                "mode": self.fltmode,
                "value": int(self.flt_start_value),
                "help": "Hide/reveal checked fields",
            }
        elif self.fltmode == "fltregex":
            result = {
                "mode": self.fltmode,
                "value": self.flt_start_value,
                "help": "Text regexp",
            }
        elif self.fltmode == "fltrange":
            result = {
                "mode": self.fltmode,
                "value": self.flt_start_value,
                "help": """Javascript code. Examples:
    "x >= 7" - value is equal or more than 7,
    "0 < x && x < 7" - value in range (0, 7)
    "x === 15" - value is equal to 15""",
            }
        else:
            result = {
                "mode": self.fltmode,
                "help": "Filter disabled",
            }

        if self.flt_form_name is not None:
            result['flt_form_name'] = self.flt_form_name

        return result

    def gen_cell_data(self, cell_data):
        result = dict()
        result['type'] = self.tp
        result['css'] = self.css

        if isinstance(cell_data, dict):
            if 'css' in cell_data and 'value' in cell_data and len(cell_data) == 2:
                result.update({'value': cell_data['value'], 'sortable': cell_data['value'], 'css': cell_data['css']})
            else:
                result.update(cell_data)
        else:
            result.update({'value': cell_data, 'sortable': cell_data})

        if self.converter:
            result['value'] = self.converter(result['value'])

        if self.tp == "int":
            result['sortable'] = int(result['sortable'])
        elif self.tp == "float":
            result['sortable'] = float(result['sortable'])

        return result

    # data can be simple type (int, float, string) or dict type (with printable and sortable elements)
    def gen_cells_data(self):
        result = []
        for cell_data in self.cells_data:
            result.append(self.gen_cell_data(cell_data))

        return result


class TListTableColumn(TTableColumn):
    def __init__(self, descr, extended_descr, tp, cells_data, converter=None, fltmode="fltdisabled", width="auto",
                 css=""):
        super(TListTableColumn, self).__init__(descr, extended_descr, tp, cells_data, converter=converter,
                                               fltmode=fltmode, width=width)
        self.css = css

    def gen_cells_data(self):
        result = []
        for cell_data in self.cells_data:
            assert (isinstance(cell_data, list))

            subcolumn = TTableColumn(self.descr, self.extended_descr, self.tp, [], converter=self.converter,
                                     fltmode=self.fltmode)
            value = map(lambda x: subcolumn.gen_cell_data(x), cell_data)
            sortable = "".join(map(lambda x: x["value"], value))
            result.append({"value": value, "sortable": sortable, "type": "list", "css": self.css})

        return result


class TCheckboxTableColumn(TTableColumn):
    def __init__(self, descr, extended_descr, tp, cells_data, converter=None, flt_form_name=None, fltmode="fltdisabled",
                 width="auto"):
        super(TCheckboxTableColumn, self).__init__(descr, extended_descr, tp, cells_data, converter=converter,
                                                   flt_form_name=flt_form_name, fltmode=fltmode, width=width)

    def gen_cell_data(self, cell_data):
        assert (isinstance(cell_data, dict))

        result = {
            "name": cell_data["name"],
            "value": cell_data["value"],
            "sortable": cell_data["value"],
            "type": "checkbox",
            "checked": cell_data["checked"],
        }

        return result


def generate_table(columns, disable_filters=False):
    assert (len(set(map(lambda x: len(x.cells_data), columns))) == 1)

    header_data = map(lambda x: x.gen_header_cell(), columns)
    filter_data = map(lambda x: x.gen_filter_cell(), columns)

    data = map(lambda x: x.gen_cells_data(), columns)
    data = map(list, zip(*data))
    data = map(lambda x: {'visible': True, 'cells': x}, data)

    result = {
        "header": header_data,
        "filters": None,
        "data": data,
    }
    if not disable_filters:
        result["filters"] = filter_data

    return result


class TFrontTable(object):
    def __init__(self, cells):
        self.cells = cells

    def mktable(self, data):
        assert (len(self.cells) == len(data))
        assert (len(set(map(lambda x: len(x), data))) == 1)

        header_data = map(lambda x: {
            "descr": x.descr,
            "help": x.extended_descr,
            "width": x.width,
        }, self.cells)

        filters_data = []
        for cell in self.cells:
            if cell.fltmode == "fltchecked":
                filters_data.append({
                    "mode": cell.fltmode,
                    "hide_unchecked": 0,
                    "help": "Hide/reveal checked fields",
                })
            elif cell.fltmode == "fltregex":
                filters_data.append({
                    "mode": cell.fltmode,
                    "regex": "",
                    "help": "Text regexp",
                })
            elif cell.fltmode == "fltrange":
                filters_data.append({
                    "mode": cell.fltmode,
                    "code": "",
                    "help": """Javascript code. Examples:
    "x >= 7" - value is equal or more than 7,
    "0 < x && x < 7" - value in range (0, 7)
    "x === 15" - value is equal to 15""",
                })
            else:
                filters_data.append({
                    "mode": cell.fltmode,
                    "help": "Filter disabled",
                })

        content = []
        for cell, cell_data in zip(self.cells, data):
            content.append(map(lambda x: cell.d(x), cell_data))

        return {
            "header": header_data,
            "filters": filters_data,
            "data": map(lambda x: {'visible': True, 'cells': list(x)}, zip(*content)),
        }


"""
Tree-related controls
"""


class TLeafElement(object):
    def __init__(self, value, tp='str', css=''):
        self.value = value
        self.tp = tp
        self.css = css

    def to_json(self):
        result = {
            'type': self.tp,
            'css': self.css,
        }

        if self.tp == 'list':
            result['value'] = map(lambda x: x.to_json(), self.value)
        else:
            if isinstance(self.value, dict):
                assert (
                'value' in self.value and 'type' not in self.value and 'css' not in self.css), "Wrong leaf element values %s" % self.value
                result.update(self.value)
            else:
                result['value'] = self.value

        return result


class TTextmoreLeafElement(TLeafElement):
    def __init__(self, value, limit=300, cut=300, css=''):
        xvalue = {'value': value, 'limit': limit, 'cut': cut}
        super(TTextmoreLeafElement, self).__init__(xvalue, tp='textmore')


class TExtenededLeafElement(object):
    def __init__(self, key_dict, value_dict):
        self.key_dict = self._convert_to_dict(key_dict)
        self.value_dict = self._convert_to_dict(value_dict)

    def _convert_to_dict(self, data):
        if isinstance(data, str):
            return {"value": data, "type": "str", "css": ""}
        elif isinstance(data, int):
            return {"value": str(data), "type": "str", "css": ""}
        elif isinstance(data, float):
            return {"value": str(data), "type": "str", "css": ""}
        elif isinstance(data, dict):
            assert ("value" in data)

            result = copy.deepcopy(data)
            if "type" not in result:
                result["type"] = "str"
            if "css" not in result:
                result["css"] = ""

            return result
        else:
            raise Exception("Unknown type <%s>" % type(data))

    def to_json(self):
        result = {
            "value": [self.key_dict, self.value_dict],
            "type": "pair",
            "css": "",
        }

        return result


class TTreeElement(object):
    def __init__(self, name, value=None, children=None):
        self.name = name
        self.value = value

        if children is None:
            self.children = []
        else:
            self.children = children

    def to_json(self):
        if len(self.children):
            self.name.css += ' printable__bold_text'

        if self.value is None:
            result = self.name.to_json()
        else:
            result = {
                'value': [self.name.to_json(), self.value.to_json()],
                'type': 'pair',
                'css': '',
            }

        if len(self.children):
            result['children'] = map(lambda x: x.to_json(), self.children)

        return result


def wrap_exception(subfunc):
    try:
        result = subfunc()
        status_tree = TTreeElement(
            TLeafElement("Status", css="printable__bold_text"),
            TLeafElement("SUCCESS", css="printable__bold_text printable__green_text"),
        )
        return result, status_tree
    except Exception:
        status_tree = TTreeElement(
            TLeafElement("Status", css="printable__bold_text"),
            TLeafElement("FAILURE", css="printable__bold_text printable__red_text"),
        )

        status_tree.children.append(TTreeElement(TLeafElement("Stack Trace"), TLeafElement(traceback.format_exc())))

        return None, status_tree
