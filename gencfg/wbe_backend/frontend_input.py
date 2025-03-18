"""
Input types in utils section. Converted to frontend input fields
"""

import os

import gencfg
from core.db import CURDB
from core.card.node import TMdDoc
from config import SCHEME_LEAF_DOC_FILE
import core.card.types

GROUP_MD_DOC = TMdDoc(os.path.join(CURDB.SCHEMES_DIR, SCHEME_LEAF_DOC_FILE))


class TMdDocPath(object):
    """
        Object, representing path in group card. On __call__ converted to documentation from doc.md
    """

    def __init__(self, path):
        self.path = path

    def __str__(self):
        return GROUP_MD_DOC.get_doc("group.yaml", self.path, TMdDoc.EFormat.HTML)


class IFrontInput(object):
    def __init__(self):
        pass

    def to_json(self, frontend_json):
        raise NotImplementedError("Function <to_json> is not implemented")

    def from_json(self, input_d, output_d):
        raise NotImplementedError("Function <from_json> is not implemented")


class ISimpleInput(IFrontInput):
    """
        Class, describing singile input value in frontend. Have fucntions to convert internal represenatation to json transferred to frontend
        and reverse function
    """

    def __init__(self, name, display_name, description, start_value, allow_empty=False, errormsg=None):
        del errormsg

        super(ISimpleInput, self).__init__()

        self.name = name
        self.display_name = display_name
        if isinstance(description, TMdDocPath):
            self.description = str(description)
        else:
            self.description = description
        self.start_value = start_value
        self.allow_empty = allow_empty

        self.type = None

        self.errormsg = None

    def to_json(self, frontend_json):
        """
            Create json to be transfered to frontend. Have several obligatory elements and few optional based on field type.
            Obligatory are:
                name: Field name (identifier)
                display_name: ???
                description: Text description of field
                value: Field value (frontend may convert and convert it back)
                type: Field type (frontend process/display field based on value of this variable)

            :type frontend_json: dict[T, T]

            :param frontend_json: dict with data to put into result json
        """

        if self.start_value is None:
            json_start_value = ""
        else:
            json_start_value = self.start_value

        result = {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "value": frontend_json.get(self.name, json_start_value),
            "type": self.type,
        }

        if self.errormsg is not None:
            result["errormsg"] = self.errormsg

        return result

    def from_json(self, input_d, output_d):
        """
            Convert <input_d> dict, got from frontend to our internal dict <output_d>. Our object is immutable

            :type input_d: dict[str, T]
            :type output_d: dict[str, T]

            :param input_d: json, received from frontend
            :param output_d: internal json (which can bed filled with some data at this moment)

            :return None:
        """

        pass


class TFrontStringInput(ISimpleInput):
    def __init__(self, name, display_name, description, start_value, tp="string", allow_empty=False,
                 value_converter=None, value_checker=None):
        assert (isinstance(start_value, str))

        super(TFrontStringInput, self).__init__(name, display_name, description, start_value, allow_empty=allow_empty)

        self.type = tp

        self.value_converter = getattr(self, "value_converter", value_converter)
        self.value_checker = getattr(self, "value_checker", value_checker)

    def from_json(self, input_d, output_d):
        v = input_d.get(self.name, self.start_value)
        if not self.allow_empty and v == "":
            raise Exception("Empty value for <%s> not allowed" % self.name)

        if self.value_converter is not None:
            v = self.value_converter(v)

        if self.value_checker is not None:
            self.value_checker(v)

        output_d[self.name] = v


class TFrontStringFromPreparedInput(TFrontStringInput):
    """
        Class presents string input with predefined list of available values. On front side input-select is used to process this
    """

    def __init__(self, name, display_name, description, start_value, avail_values, tp="string", allow_empty=False,
                 value_converter=None, value_checker=None, multiple=True):
        super(TFrontStringFromPreparedInput, self).__init__(name, display_name, description, start_value, tp=tp,
                                                            allow_empty=allow_empty, value_converter=value_converter,
                                                            value_checker=value_checker)

        self.avail_values = avail_values
        self.multiple = multiple

    def to_json(self, frontend_input):
        result = super(TFrontStringFromPreparedInput, self).to_json(frontend_input)
        result["avail_values"] = self.avail_values
        result["multiple"] = self.multiple

        return result


class TFrontCIMInput(TFrontStringFromPreparedInput):
    """
        Class presents input for ctype/itype/metaprj with all checks included
    """

    class ECheckType(object):
        def __init__(self):
            pass

        CTYPE = "ctype"
        ITYPE = "itype"
        METAPRJ = "metaprj"
        ALL = [CTYPE, ITYPE, METAPRJ]

    def __init__(self, name, display_name, description, start_value, db, checktype, tp="string", allow_empty=False,
                 multiple=True):
        self.db = db
        self.checktype = checktype

        if self.checktype == TFrontCIMInput.ECheckType.CTYPE:
            avail_values = map(lambda x: x.name, self.db.ctypes.get_ctypes())
        elif self.checktype == TFrontCIMInput.ECheckType.ITYPE:
            avail_values = map(lambda x: x.name, self.db.itypes.get_itypes())
        elif self.checktype == TFrontCIMInput.ECheckType.METAPRJ:
            avail_values = self.db.constants.METAPRJS.keys()
        else:
            raise Exception("Unknown checktype <%s>" % checktype)

        super(TFrontCIMInput, self).__init__(name, display_name, description, start_value, avail_values, tp=tp,
                                             allow_empty=allow_empty, multiple=multiple)

    def value_converter(self, value):
        if self.multiple:
            return map(lambda x: x.strip(), value.split(","))
        else:
            return value.strip()

    def value_checker(self, checkvalue):
        """
            Check if ctype is in list of available ctypes or not

            :type checkvalue: list[str]

            :param checkvalue: when multiple enabled checkvalue is <list of strings>, otherwise <checkvalue> is string
            :return None: just raise exception if something goes wrong
        """

        if self.multiple:
            values = checkvalue
        else:
            values = [checkvalue]

        etalon_group = self.db.groups.get_group("MSK_RESERVED")  # FIXME
        for value in values:
            if self.checktype == TFrontCIMInput.ECheckType.CTYPE:
                status = core.card.types.CtypeType().validate_for_update(etalon_group, value)
            elif self.checktype == TFrontCIMInput.ECheckType.ITYPE:
                status = core.card.types.ItypeType().validate_for_update(etalon_group, value)
            elif self.checktype == TFrontCIMInput.ECheckType.METAPRJ:
                status = core.card.types.MetaprjType().validate_for_update(etalon_group, value)

            if status.status == core.card.types.EStatuses.STATUS_FAIL:
                raise Exception(status.reason)


class TFrontCheckboxInput(ISimpleInput):
    def __init__(self, name, display_name, description, start_value):
        assert (isinstance(start_value, bool))

        super(TFrontCheckboxInput, self).__init__(name, display_name, description, start_value)
        self.type = "bool"

    def to_json(self, frontend_json):
        result = super(TFrontCheckboxInput, self).to_json(frontend_json)
        result["value"] = int(result["value"])

        return result

    def from_json(self, input_d, output_d):
        if self.name in input_d:
            if input_d[self.name] == '1':
                output_d[self.name] = True
            else:
                output_d[self.name] = False
        else:
            output_d[self.name] = self.start_value


class TFrontNumberInput(ISimpleInput):
    def __init__(self, name, display_name, description, start_value, tp="float", allow_empty=False):
        if allow_empty:
            assert (start_value is None or isinstance(start_value, (int, float)))
        else:
            assert (isinstance(start_value, (int, float)))

        super(TFrontNumberInput, self).__init__(name, display_name, description, start_value, allow_empty=allow_empty)
        self.type = "number"

        self.tp = tp

    def from_json(self, input_d, output_d):
        v = input_d.get(self.name, self.start_value)

        if self.allow_empty and self.start_value is None and v == "":
            output_d[self.name] = None
            return

        try:
            if self.tp == "int":
                v = int(v)
            elif self.tp == "float":
                v = float(v)
            else:
                raise Exception("Unknown tp %s in TFrontNumberInput" % self.tp)
        except ValueError:
            raise Exception("Can not convert <%s> to type <%s>" % (v, self.tp))

        output_d[self.name] = v


class TFrontChoiceInput(ISimpleInput):
    def __init__(self, name, display_name, description, start_value, items, allow_empty=False):
        super(TFrontChoiceInput, self).__init__(name, display_name, description, start_value)

        self.items = items
        self.type = "choice"

        self.allow_empty = allow_empty

    def to_json(self, frontend_json):
        result = super(TFrontChoiceInput, self).to_json(frontend_json)

        result['items'] = map(lambda (name, value): {'title': name, 'value': value}, self.items)
        if self.allow_empty:
            result['items'] = [{"title": "None", "value": ""}] + result['items']

        return result

    def from_json(self, input_d, output_d):
        if self.name in input_d:
            if input_d[self.name] == "":
                output_d[self.name] = self.start_value
            else:
                output_d[self.name] = input_d[self.name]
        else:
            output_d[self.name] = self.start_value


class TDescrInput(IFrontInput):
    def __init__(self, name, display_name):
        super(TDescrInput, self).__init__()

        self.name = name
        self.display_name = display_name

    def to_json(self, frontend_json):
        result = {
            'name': self.name,
            'display_name': self.display_name,
            'type': 'descr',
        }

        return result

    def from_json(self, input_d, output_d):
        pass


# class for rendering output leaf element as part of input
class TPrintableInput(IFrontInput):
    def __init__(self, name, printable_value):
        super(TPrintableInput, self).__init__()
        self.name = name
        self.printable_value = printable_value

    def to_json(self, frontend_json):
        result = {
            'name': self.name,
            'type': 'printable',
            'value': map(lambda x: x.to_json(), self.printable_value),
        }

        return result

    def from_json(self, input_d, output_d):
        pass


class TTableInput(IFrontInput):
    def __init__(self, name, table_columns):
        super(IFrontInput, self).__init__()

        self.name = name
        self.table_columns = table_columns
        self.type = 'table'

    def to_json(self, frontend_json=None):
        # update filter values
        if frontend_json is None:
            frontend_json = {}
        for column in self.table_columns:
            if column.flt_form_name is not None and frontend_json.get(column.flt_form_name, None) is not None:
                column.flt_start_value = frontend_json.get(column.flt_form_name, None)

        # update list of checked hosts
        checked_values = frontend_json.get(self.name, [])
        if not isinstance(checked_values, list):
            checked_values = [checked_values]

        from frontend_output import generate_table
        table_data = generate_table(self.table_columns)

        for row in table_data['data']:
            for cell in row['cells']:
                if cell['type'] == 'checkbox':
                    cell['checked'] = cell['value'] in checked_values

        result = {
            'name': self.name,
            'type': self.type,
            'value': table_data,
        }

        return result

    def from_json(self, input_d, output_d):
        pass


# convert internal input elements to json for frontend
def convert_input_to_frontent(params, frontend_json):
    return map(lambda x: x.to_json(frontend_json), params)


# convert input form fields to internal structures
def convert_input_from_frontend(params, form_params, raise_errors=True):
    errors = {}
    result = {}
    for param in params:
        try:
            param.from_json(form_params, result)
        except Exception, e:
            if raise_errors:
                raise
            else:
                errors[param.name] = str(e)

    if raise_errors:
        return result
    else:
        return result, errors
