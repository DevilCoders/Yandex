import re
import exceptions

def get_field_value_dictionary(log_string, delimiter_symbol = "\t", equal_symbol = "="):
    signle_field_value_regexp = re.compile("([A-Za-z0-9_-]*)%(0)s(.*)" % {"0": equal_symbol})
    field_dictionary = {}
    
    field_value_list = log_string.split(delimiter_symbol)
    
    for field_value in field_value_list:
        field_value = field_value.strip()
        
        if len(field_value) == 0:
            continue
        match = signle_field_value_regexp.match(field_value)
        if not match == None:
            LogFieldStringFormatter.add_field_value(match.group(1), match.group(2), field_dictionary)
    return field_dictionary

class LogFieldStringFormatter:
    def __init__(self, delimiter_symbol = "\t"):
        self.delimiter_symbol = delimiter_symbol
        self.field_value_dictionary = {}
        self.adding_order_field_names = []
        
    # field_value can be list
    def add_field(self, field_name, field_value):
        self.add_field_value(field_name, field_value, self.field_value_dictionary)
        if self.adding_order_field_names.count(field_name) == 0:
            self.adding_order_field_names.append(field_name)
            
    def add_list_field(self, field_name, field_list_value):
        if self.adding_order_field_names.count(field_name) == 0:
            self.adding_order_field_names.append(field_name)
        self.field_value_dictionary[field_name] = field_list_value
        
    def to_string(self):
        result_str = ""
        for field_name in self.adding_order_field_names:
            if len(result_str) > 0:
                result_str += self.delimiter_symbol
                
            if isinstance(self.field_value_dictionary[field_name], list):
                if len(self.field_value_dictionary[field_name]) > 0:
                    result_str = result_str + "%(0)s=" % {"0": field_name} + ("\t%(0)s=" % {"0": field_name}).join(self.field_value_dictionary[field_name])
            elif not self.field_value_dictionary[field_name] is None:
                result_str = result_str + "%(0)s=" % {"0": field_name} + str(self.field_value_dictionary[field_name])
        return result_str
        
    @staticmethod
    def add_field_value(field_name, field_value, field_dictionary):
        if not field_value == None and type(field_value).__name__ == "str":
            field_value = field_value.strip()
        if field_dictionary.has_key(field_name):
            if not isinstance(field_dictionary[field_name], list):
                value_list = []
                value_list.append(field_dictionary[field_name])
                field_dictionary[field_name] = value_list
            field_dictionary[field_name].append(field_value)
        else:
            field_dictionary[field_name] = field_value