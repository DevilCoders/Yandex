from pyparsing import ParseException
import itertools
import json


class JsonToUpdateGenerator(object):
    template = """\
static inline {update_type} JsonTo{update_name}(const TString& s) {{
    NProtobufJson::TJson2ProtoConfig config;
    config.SetUseJsonName(true);
    config.SetCastFromString(true);
    return NProtobufJson::Json2Proto<{update_type}>(s, config);
}}
"""

    def __init__(self, update_name, update_type):
        self.update_name = update_name
        self.update_type = update_type

    def generate(self):
        return self.template.format(
            update_name=self.update_name,
            update_type=self.update_type)


class UpdateToJsonGenerator(object):
    template = """\
static inline TString {update_name}ToJson(const {update_type}& update) {{
    NJson::TJsonValue json;
    NProtobufJson::TProto2JsonConfig config;
    config.SetUseJsonName(true);
    NProtobufJson::Proto2Json(update, json, config);
    return WriteJson(JsonToFedorson(json));
}}
"""

    def __init__(self, update_name, update_type):
        self.update_name = update_name
        self.update_type = update_type

    def generate(self):
        return self.template.format(
            update_name=self.update_name,
            update_type=self.update_type)


class EntityRowToUpdateGenerator(object):
    template = """\
static inline {update_type} Create{update_name}(const {row_type}& row) {{
    {update_type} updateProto;
    updateProto.SetType("ugcupdate");
    updateProto.SetVersion("1.0");
    updateProto.SetUserId(row.Get{root_key}());

    auto& updateRow = *updateProto.Add{vtable}();

    {set_key_fields}

    {set_fields}

    return updateProto;
}}
"""

    def __init__(self, update_name, update_type, row_type, root_key, vtable):
        self.update_name = update_name
        self.update_type = update_type
        self.row_type = row_type
        self.root_key = root_key
        self.vtable = vtable
        self.key_fields = []
        self.fields = []

    def add_key_field(self, field):
        self.key_fields.append(field)

    def add_field(self, field):
        self.fields.append(field)

    def generate(self):
        set_key_fields = """\
updateRow.SetKey(row.Get{}());
""".format(self.key_fields[0])

        for field in self.key_fields[1:]:
            set_key_fields += """\
updateRow.Set{field}(row.Get{field}());
""".format(field=field)

        set_fields = ""
        for field in self.fields:
            set_fields += """\
if (row.Has{field}()) {{
    updateRow.Set{field}(row.Get{field}());
}}
""".format(field=field)

        return self.template.format(
            update_name=self.update_name,
            update_type=self.update_type,
            row_type=self.row_type,
            root_key=self.root_key,
            vtable=self.vtable,
            set_key_fields=set_key_fields,
            set_fields=set_fields,
        )


class ProtoGenerator(object):
    def __init__(self, name):
        self.name = name
        self.fields = []

    def add_field(self, field):
        self.fields.append(field)

    def add_fields(self, fields):
        self.fields += fields

    def generate(self):
        result = "message " + self.name + " {\n"

        oneof = False
        indent = "    "
        for i, field in enumerate(self.fields):
            if field[0] == "oneof":
                if not oneof:
                    # open oneof
                    result += indent + "oneof " + field[1] + " {\n"
                    indent += "    "
                    oneof = True

                field = field[2:]
            else:
                if oneof:
                    # close oneof
                    indent = indent[:-4]
                    result += indent + "}\n"
                    oneof = False

            if not oneof and field[0] != "repeated":
                field = ["optional"] + field

            options = ""
            if field[-1].startswith("["):
                options = " " + field[-1]
                field = field[:-1]

            result += "{indent}{field} = {index}{options};\n".format(
                indent=indent, field=" ".join(field), index=i+1, options=options)

        if oneof:
            # close oneof
            indent = indent[:-4]
            result += indent + "}\n"

        result += "}\n"
        return result


class SchemaGenerator(object):
    def __init__(self, schema):
        if "SCHEMA" not in schema:
            raise ParseException("no schema description")
        if len(schema["SCHEMA"]) > 1:
            raise ParseException("more than one schema description")

        self.scname, self.schema = next(iter(schema["SCHEMA"].items()))

        self.all_keys = []
        self.protos = []
        self.update_protos = {}
        self.update_selectors = {}
        self.row_to_update_generators = []
        self.json_to_update_generators = []
        self.update_to_json_generators = []
        self.rows = []
        self._traverse_tables(self.schema)

    def generate_proto(self):
        result = """\
syntax = "proto2";

package N{};
""".format(self.scname)

        for proto in itertools.chain(self.protos, self.update_protos.values()):
            result += "\n" + proto.generate()

        entity_proto = ProtoGenerator("T{}Entity".format(self.scname))
        for row_type, row_name in self.rows:
            entity_proto.add_field(["oneof", "Row", row_type, row_name])
        result += "\n" + entity_proto.generate()

        return result

    def generate_cpp(self):
        result = """\
#pragma once

#define INCLUDE_SCHEMA_IMPL_H
#include <kernel/ugc/schema/cpp/schema-impl.h>
#undef INCLUDE_SCHEMA_IMPL_H

#include <library/cpp/json/json_writer.h>

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>

namespace N{scname} {{
    constexpr char {scname}SchemaString[] = R"_({schema_string})_";

    struct T{scname}Schema: public TSchema {{
        T{scname}Schema()
            : TSchema({scname}SchemaString)
        {{
        }}
    }};

    static inline google::protobuf::Message* FindEntityRow(T{scname}Entity& entity, const TString& rowName) {{
        {find_row_loop}
        return nullptr;
    }}

    static inline google::protobuf::Message* GetEntityRow(T{scname}Entity& entity) {{
        auto rowCase = entity.GetRowCase();
        {get_row_loop}
        return nullptr;
    }}

    static inline T{scname}Entity Create{scname}Entity(TStringBuf path) {{
        return CreateSchemaEntity<T{scname}Schema, T{scname}Entity>(path);
    }}

    {row_to_update_generators}

    {update_selectors}

    {json_to_update_generators}

    {update_to_json_generators}
}}
"""

        find_row_loop = ""
        for row_type, row_name in self.rows:
            find_row_loop += """if (rowName == \"{row_name}\") {{
            return entity.Mutable{row_name}();
        }}""".format(row_name=row_name)

        get_row_loop = ""
        for row_type, row_name in self.rows:
            get_row_loop += """if (rowCase == T{scname}Entity::k{row_name}) {{
            return entity.Mutable{row_name}();
        }}""".format(row_name=row_name, scname=self.scname)

        for k in self.update_selectors.keys():
            self.update_selectors[k] += "        ythrow yexception() << \"invalid proto\";\n    }"

        return result.format(
            scname=self.scname,
            schema_string=json.dumps(self.schema, sort_keys=True, separators=(',', ':')),
            find_row_loop=find_row_loop,
            get_row_loop=get_row_loop,
            update_selectors="\n".join(self.update_selectors.values()),
            row_to_update_generators="".join(map(lambda x: x.generate(), self.row_to_update_generators)),
            json_to_update_generators="".join(map(lambda x: x.generate(), self.json_to_update_generators)),
            update_to_json_generators="".join(map(lambda x: x.generate(), self.update_to_json_generators)),
        )

    def _traverse_tables(self, schema, keys=[], root_table=None, vtable=None):
        for table_name, table in schema.get("TABLE", {}).items():
            key = table.get("KEY")
            if key is None:
                raise ParseException("table '{}' has no key".format(table_name))
            current_keys = keys + [key]
            self.all_keys.append(key)

            if len(keys) == 0:
                # we are at the root table
                root_table = table_name
                update_proto = ProtoGenerator("T" + root_table + "UpdateProto")
                update_proto.add_field(["string", "Type"])
                update_proto.add_field(["string", "Version"])
                update_proto.add_field(["string", "App"])
                update_proto.add_field(["uint64", "Time"])
                update_proto.add_field(["string", "UpdateId"])
                update_proto.add_field(["string", key])
                if key == "UserId":
                    # XXX: dirty hack
                    update_proto.add_field(["string", "VisitorId"])
                    update_proto.add_field(["string", "DeviceId"])
                self.update_protos[key] = update_proto

                self.update_selectors[key] = """\
static inline T{root_table}UpdateProto Create{root_table}UpdateProto(const google::protobuf::Message& msg) {{
""".format(root_table=root_table)

                update_name = "{root_table}UpdateProto".format(root_table=root_table)
                update_type = "T{update_name}".format(update_name=update_name)
                json_to_update_generator = JsonToUpdateGenerator(update_name, update_type)
                update_to_json_generator = UpdateToJsonGenerator(update_name, update_type)
                self.json_to_update_generators.append(json_to_update_generator)
                self.update_to_json_generators.append(update_to_json_generator)

            elif len(keys) == 1:
                # we are at the virtual (second level) table
                vtable = (table_name, table)

            if "ROW" in table:
                row_name = table_name
                assert len(table["ROW"]) == 1
                row_type, row_fields = next(iter(table["ROW"].items()))

                row_proto = ProtoGenerator(row_type)
                for key in current_keys:
                    row_proto.add_field(["string", key])
                row_proto.add_fields(row_fields)
                self.protos.append(row_proto)

                update_row_type = row_type + "UpdateRow"
                update_row_proto = ProtoGenerator(update_row_type)
                update_row_proto.add_field(["string", "Key"])
                for k in current_keys[2:]:
                    update_row_proto.add_field(["string", k])
                update_row_proto.add_fields(row_fields)

                update_name = "{root_table}UpdateProto".format(root_table=root_table)
                update_type = "T{update_name}".format(update_name=update_name)

                row_to_update_generator = EntityRowToUpdateGenerator(update_name, update_type, row_type, keys[0], vtable[0])
                if len(keys) > 1:
                    row_to_update_generator.add_key_field(keys[1])
                    for field in current_keys[2:]:
                        row_to_update_generator.add_key_field(field)
                for field in row_fields:
                    row_to_update_generator.add_field(field[-1])

                if vtable:
                    self.protos.append(update_row_proto)
                    table_field = ["repeated", update_row_type, vtable[0]]
                    if "JSON_NAME" in vtable[1]:
                        table_field.append("[json_name = \"{}\"]".format(vtable[1]["JSON_NAME"]))
                    self.update_protos[keys[0]].add_field(table_field)
                    self.row_to_update_generators.append(row_to_update_generator)
                    self.update_selectors[keys[0]] += """\
        const auto* row{row_name} = dynamic_cast<const {row_type}*>(&msg);
        if (row{row_name}) {{
            return Create{root_table}UpdateProto(*row{row_name});
        }}
""".format(row_name=row_name, row_type=row_type, root_table=root_table)

                self.rows.append((row_type, row_name))

            self._traverse_tables(table, current_keys, root_table, vtable)
