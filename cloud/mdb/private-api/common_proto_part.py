import sys
from copy import copy
import os.path
import sys
import argparse

COMMENT = '// ALARMA!!! DO NOT EDIT MANUALLY\n'


class Element:
    def __init__(self):
        self.name = ''
        self.header = []
        self.body = None
        self.closure = ''


class Field(Element):
    def __init__(self):
        super().__init__()
        self.number = 0
        self.repeated = False
        self.template = ''
        self.comment = False
        self.type = ''
        self.message_to_copy = None
        self.hidden = False

    @staticmethod
    def parse(lines, i, comment=None, enum=False):
        line = lines[i]
        field = Field()
        if comment:
            for l in comment:
                field.header.append(l)

        units = line.strip().split()
        name_index = 0
        if units[0] == '//':
            field.comment = True
            name_index += 1
            if units[1] == 'common_proto_part':
                field.message_to_copy = units[2]
                field.template = line
                return field, i
        if units[name_index] == 'repeated':
            field.repeated = True
            name_index += 1

        field.template = ''
        for e in line:
            if e.isspace():
                field.template += e
            else:
                break
        if field.comment and units[1] == 'common_proto_part':
            field.message_to_copy = units[2]
            return field, i
        field.template += '{comment}{repeated}{type}{name} = {number}'
        if not enum:
            name_index += 1
            field.type = units[name_index - 1]
        field.name = units[name_index]
        if units[name_index + 2].endswith(';'):
            field.number = int(units[name_index + 2][:-1])
        else:
            field.number = int(units[name_index + 2])
        field.template += line[(line.find('=') + len(str(field.number)) + 2) :].rstrip()
        if i + 1 < len(lines) and len(lines[i + 1].strip()) == 0:
            i += 1
        return field, i

    def __str__(self):
        if self.hidden:
            return ''
        if self.message_to_copy:
            return '// common_proto_part {0}'.format(self.message_to_copy)
        result = ''
        for c in self.header:
            result += str(c)
        result += str(
            self.template.format(
                repeated='repeated ' if self.repeated else '',
                name=self.name,
                number=self.number,
                comment='// ' if self.comment else '',
                type=self.type + ' ' if self.type else '',
            )
        )
        result += self.closure
        return result

    def __eq__(self, other):
        return str(self) == str(other)

    def __copy__(self):
        e = Field()
        e.name = self.name
        e.number = self.number
        e.header = []
        for h in self.header:
            e.header.append(copy(h))
        e.template = copy(self.template)
        e.repeated = self.repeated
        e.closure = self.closure
        e.type = self.type
        e.comment = self.comment
        return e

    def __hash__(self):
        return hash(str(self))


class Message(Element):
    def __init__(self):
        super().__init__()
        self.body = []
        self.elements_by_numbers = dict()
        self.elements_by_names = dict()
        self.max_number = -1
        self.fields_without_numbers = 0

    def intersect(self, other):
        result = Message()
        result.components = set(self.body) & set(other.body)

        return result

    def __str__(self):
        result = 'message {0}'.format(self.name) + ' {\n'
        result += '\n\n'.join([str(b) for b in self.body if not b.hidden]) + '\n'
        result += self.closure
        return result

    def __hash__(self):
        return hash((self.name, tuple(self.header), tuple(self.body), self.closure.strip()))

    def prepare(self):
        for e in self.body:
            if isinstance(e, Field):
                if e.number > 0:
                    self.elements_by_numbers[e.number] = e
                    self.max_number = max(self.max_number, e.number)
                else:
                    self.fields_without_numbers += 1
            self.elements_by_names[e.name] = e
        self.body.sort(
            key=lambda x: '0' + x.name
            if isinstance(x, Enum)
            else ('2' if x.number == -1 else '1' + '0' * (10 - len(str(x.number))) + str(x.number))
        )

    def hide(self, element):
        self.elements_by_names[element.name].hidden = True


class Enum(Element):
    def __init__(self):
        super().__init__()
        self.body = []
        self.hidden = False

    def __eq__(self, other):
        if self.name != other.name:
            return False
        if len(self.body) != len(other.body):
            return False
        for i in range(len(self.body)):
            if self.body[i] != other.body[i]:
                return False
        return True

    def __hash__(self):
        return hash((self.name, tuple(self.header), tuple(self.body), self.closure.strip()))

    def __str__(self):
        if self.hidden:
            return ''
        result = ''
        for b in self.header:
            result += str(b)
        result += '\n\n'.join([str(b) for b in self.body if not b.hidden]) + '\n'
        result += self.closure
        return result

    def __copy__(self):
        enum = Enum()
        enum.name = self.name
        enum.header = []
        for e in self.header:
            enum.header.append(copy(e))
        enum.closure = self.closure
        enum.body = []
        for e in self.body:
            enum.body.append(copy(e))
        return enum


class File(Element):
    def __init__(self):
        super().__init__()
        self.body = []

    def __str__(self):
        result = ''
        for b in self.header:
            result += str(b)
        result += '\n\n'.join([str(b) for b in self.body]) + '\n'
        result += self.closure
        return result

    @staticmethod
    def parse(lines, messages, filename=None):
        file = File()
        stack = [file]
        comment = []
        i = 0
        if len(lines) == 0:
            return file
        while 'message' not in lines[i]:
            file.header.append(lines[i])
            i += 1
        while i < len(lines):
            line = lines[i]
            new_struct = False
            if line.strip().startswith('message'):
                stack.append(Message())
                new_struct = True
            elif line.strip().startswith('enum'):
                stack.append(Enum())
                new_struct = True
            elif '}' in line:
                stack[-1].closure = lines[i].rstrip()
                if i + 1 < len(lines) and len(lines[i + 1].strip()) == 0:
                    i += 1
                item = stack.pop()
                messages[item.name] = item
                stack[-1].body.append(item)
                comment = []
            elif '=' in line or 'common_proto_part' in line:
                field, i = Field.parse(lines, i, comment=comment, enum=isinstance(stack[-1], Enum))
                stack[-1].body.append(field)
                comment = []

            elif line.strip().startswith('//'):
                comment.append(line)
            elif len(line.strip()) == 0:
                # just skipping empty line like that
                pass
            else:
                raise Exception("Parsing problems in line number {0} ('{1}') in file {2}".format(i, line, filename))

            if new_struct:
                stack[-1].header = []
                for l in comment:
                    stack[-1].header.append(l)
                stack[-1].header.append(line)
                comment = []
                stack[-1].name = line.strip().split()[1]
            i += 1
        return file


def compress_messages(messages, common_message_name):
    common = Message()
    common.name = common_message_name
    common.closure = '}'
    numbers_to_elements = dict()
    names_to_elements = dict()

    for m in messages:
        m.prepare()
        numbers_to_elements |= m.elements_by_numbers
        names_to_elements |= m.elements_by_names

    # enums
    for element in messages[0].body:
        if not isinstance(element, Enum):
            continue
        in_all_messages = True
        for m in messages[1:]:
            if element.name not in m.elements_by_names or element != m.elements_by_names[element.name]:
                in_all_messages = False
        if in_all_messages:
            element_copy = copy(element)
            common.body.append(element_copy)

    # fields
    for element in messages[0].body:
        if not isinstance(element, Field):
            continue
        in_all_messages = True
        number = element.number
        for m in messages[1:]:
            if element.name not in m.elements_by_names:
                in_all_messages = False
                continue
            other_element = m.elements_by_names[element.name]
            if number != -1 and other_element.number != -1 and number != other_element.number:
                in_all_messages = False
                continue
            if element.repeated != other_element.repeated:
                in_all_messages = False
                continue
            if number == -1 and other_element.number > 0:
                number = other_element.number
        for m in messages:
            if number in m.elements_by_numbers and (
                m.elements_by_numbers[number].name != element.name
                or m.elements_by_numbers[number].repeated != element.repeated
            ):
                in_all_messages = False
                continue
        if in_all_messages:
            common_element = copy(element)
            common_element.number = number
            element.number = number
            common.body.append(common_element)
    for element in common.body:
        if isinstance(element, Enum):
            for m in messages:
                m.elements_by_names[element.name].hidden = True
        elif isinstance(element, Field):
            if element.number != -1:
                for m in messages:
                    m.hide(element)
                common.elements_by_numbers[element.number] = copy(element)
                common.elements_by_names[element.name] = copy(element)

    free_number = 1
    for element in common.body:
        if isinstance(element, Field) and element.number == -1:
            while True:
                is_free = True
                for m in messages:
                    if free_number in m.elements_by_numbers:
                        is_free = False
                        break
                if is_free:
                    element.number = free_number
                    for m in messages:
                        m.hide(element)
                    common.elements_by_numbers[element.number] = element
                    common.elements_by_names[element.name] = element
                    break
                free_number += 1
    for m in messages:
        common_field = Field()
        common_field.message_to_copy = common_message_name
        m.body = [common_field] + m.body
    return common


def enumerate_fields_in_messages(messages):
    for m in messages:
        m.prepare()
    max_index = max([m.max_number for m in messages]) + max([m.fields_without_numbers for m in messages])
    free_numbers = []
    for i in range(1, max_index + 1):
        name = None
        is_unique_name_by_number = True
        for m in messages:
            current_element = m.elements_by_numbers.get(i)
            if not current_element:
                continue
            if name and name != current_element.name:
                is_unique_name_by_number = False
                break
            name = m.elements_by_numbers.get(i).name
        if is_unique_name_by_number and not name:
            free_numbers.append(i)
        if name and is_unique_name_by_number:
            for m in messages:
                element = m.elements_by_names.get(name)
                if not element:
                    continue
                if element.number != -1:
                    continue
                element.number = i
                m.prepare()
    all_fields_by_names = dict()
    all_fields_by_numbers = dict()
    for m in messages:
        for name, element in m.elements_by_names.items():
            if not isinstance(element, Field):
                continue
            if element.name not in all_fields_by_names:
                all_fields_by_names[element.name] = set()
            all_fields_by_names[element.name].add(element.number)
            if element.number == -1:
                continue
            if element.number not in all_fields_by_numbers:
                all_fields_by_numbers[element.number] = set()
            all_fields_by_numbers[element.number].add(element.name)
    i = 0
    for name, numbers in all_fields_by_names.items():
        if -1 in numbers:
            new_number = -1
            for n in numbers:
                if tuple(all_fields_by_numbers.get(n, [None])) == (name,):
                    new_number = n
                    break
            if new_number == -1:
                new_number = free_numbers[i]
                i += 1
            for m in messages:
                if name in m.elements_by_names:
                    if m.elements_by_names[name].number == -1:
                        m.elements_by_names[name].number = new_number
    for m in messages:
        m.prepare()


def expand_message(source_name, destination, name_to_message):
    # destination.prepare()
    if source_name not in name_to_message:
        raise Exception("Message {0} expected, but not found.".format(source_name))
    source = name_to_message[source_name]
    for element in source.body:
        if element.hidden:
            continue
        if isinstance(element, Field) and element.message_to_copy:
            expand_message(element.message_to_copy, source, name_to_message)
            element.hidden = True
            continue
        # if element.name in destination.elements_by_names:
        #    continue
        destination.body.append(element)


def generate_protos_from_common_part(messages):
    name_to_message = dict()
    for m in messages:
        name_to_message[m.name] = m
    for m in messages:
        i = 0
        while i < len(m.body):
            element = m.body[i]
            if isinstance(element, Field) and element.message_to_copy:
                expand_message(element.message_to_copy, m, name_to_message)
                element.hidden = True
            i += 1
        m.prepare()


def compress(filenames, common_file_name, message_names, common_message_name):
    messages = dict()
    files = dict()
    for filename in filenames:
        with open(filename, 'r') as f:
            file_lines = f.readlines()
        files[filename] = File.parse(file_lines, messages, filename=filename)
        files[filename].name = filename

    if os.path.exists(common_file_name):
        with open(common_file_name, 'r') as f:
            file_lines = f.readlines()
        common_file = File.parse(file_lines, dict(), filename=common_file_name)
    else:
        common_file = File()
    common_file.name = common_file_name

    common_message = compress_messages([messages[name] for name in message_names], common_message_name)

    common_file_messages = dict()
    for i in range(len(common_file.body)):
        m = common_file.body[i]
        common_file_messages[m.name] = i
    if common_message.name in common_file_messages:
        common_file.body[common_file_messages[common_message.name]] = common_message
    else:
        common_file.body.append(common_message)

    for name in message_names:
        message = messages[name]
        if not isinstance(message, Message):
            continue
        if name in common_file_messages:
            common_file.body[common_file_messages[name]] = message
        else:
            common_file.body.append(message)

    with open(common_file.name, 'w') as f:
        f.write(str(common_file))


def decompress(filenames, common_file_name, message_names, common_message_name):
    message_names += [common_message_name]
    messages = dict()
    files = dict()
    for filename in filenames:
        with open(filename, 'r') as f:
            file_lines = f.readlines()
        files[filename] = File.parse(file_lines, dict(), filename=filename)
        files[filename].name = filename
    with open(common_file_name, 'r') as f:
        file_lines = f.readlines()
    File.parse(file_lines, messages, filename=common_file_name)

    enumerate_fields_in_messages([messages[name] for name in message_names])
    generate_protos_from_common_part([messages[name] for name in message_names])
    for filename, file in files.items():
        for i in range(len(file.body)):
            file.body[i] = messages.get(file.body[i].name, file.body[i])

    for filename in filenames:
        with open(filename, 'w') as f:
            if not files[filename].header[0].startswith(COMMENT.split('\n')[0]):
                f.write(COMMENT + '\n\n')
            f.write(str(files[filename]))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--compress', action='store_true', help='compress proto messages')
    parser.add_argument('-d', '--decompress', action='store_true', help='decompress and enumerate proto messages')
    parser.add_argument('files', type=str, help='files to read, separated by ","')
    parser.add_argument('common_file', type=str, help='name of the file with common messages')
    parser.add_argument('messages', type=str, help='messages to read, separated by ","')
    parser.add_argument('common_message', type=str, help='name of the common message')
    args = parser.parse_args()

    if int(args.compress) + int(args.decompress) != 1:
        print("Use exactly one of --compress(-c)/--decompress(-d) flags")
        sys.exit(1)
    if args.compress:
        compress(args.files.split(','), args.common_file, args.messages.split(','), args.common_message)
    if args.decompress:
        decompress(args.files.split(','), args.common_file, args.messages.split(','), args.common_message)
