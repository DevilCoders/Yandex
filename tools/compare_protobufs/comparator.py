from google.protobuf.descriptor import FieldDescriptor


def _compare_single_field(first, second, field_type, precision, fields_to_ignore):
    is_proto = (field_type == FieldDescriptor.TYPE_MESSAGE)
    is_float = (
        field_type == FieldDescriptor.TYPE_FLOAT or
        field_type == FieldDescriptor.TYPE_DOUBLE
    )
    if is_proto:
        return compare_protobufs(first, second, precision, fields_to_ignore)
    elif is_float:
        return abs(first - second) <= precision
    else:
        return first == second


def _fields_filter(fields, fields_to_ignore=set()):
    filtered_fields = list()
    for field in fields:
        descriptor, _ = field
        if descriptor.full_name not in fields_to_ignore:
            filtered_fields.append(field)
    return filtered_fields


def compare_protobufs(first, second, precision=1e-5, fields_to_ignore=set()):
    first_fields = _fields_filter(first.ListFields(), fields_to_ignore)
    second_fields = _fields_filter(second.ListFields(), fields_to_ignore)
    first_fields.sort(key=lambda x: x[0].name)
    second_fields.sort(key=lambda x: x[0].name)
    first_fields_names = [descriptor.name for descriptor, _ in first_fields]
    second_fields_names = [descriptor.name for descriptor, _ in second_fields]
    if first_fields_names != second_fields_names:
        return False

    for (first_descriptor, first_value), (second_descriptor, second_value) in zip(first_fields, second_fields):
        is_repeated = (first_descriptor.label == FieldDescriptor.LABEL_REPEATED)
        is_map_entry = (
            first_descriptor.type == FieldDescriptor.TYPE_MESSAGE and
            first_descriptor.message_type.has_options and
            first_descriptor.message_type.GetOptions().map_entry
        )

        if is_map_entry:
            if first_value.keys() != second_value.keys():
                return False
            for key in first_value.keys():
                if not compare_protobufs(
                    first_value.GetEntryClass()(key=key, value=first_value[key]),
                    second_value.GetEntryClass()(key=key, value=second_value[key]),
                    precision,
                    fields_to_ignore
                ):
                    return False
        elif is_repeated:
            if len(first_value) != len(second_value):
                return False
            for i in range(len(first_value)):
                if not _compare_single_field(first_value[i], second_value[i], first_descriptor.type, precision, fields_to_ignore):
                    return False
        else:
            if not _compare_single_field(first_value, second_value, first_descriptor.type, precision, fields_to_ignore):
                return False
    return True
