from kernel.ugc.schema.compiler.lib import SchemaGenerator, SchemaParser


def test_key_proto():
    p = SchemaParser("""
        SCHEMA Schema {
            TABLE Table1 {
                KEY Key1;

                TABLE Table2 {
                    KEY Key2;
                }
            }

            TABLE Table3 {
                KEY Key3;

                TABLE Table4 {
                    KEY Key4;

                    TABLE Table5 {
                        KEY Key5;
                    }
                }
            }
        }
    """)

    expected = """\
syntax = "proto2";

package NSchema;

message TTable1UpdateProto {
    optional string Type = 1;
    optional string Version = 2;
    optional string App = 3;
    optional uint64 Time = 4;
    optional string UpdateId = 5;
    optional string Key1 = 6;
}

message TTable3UpdateProto {
    optional string Type = 1;
    optional string Version = 2;
    optional string App = 3;
    optional uint64 Time = 4;
    optional string UpdateId = 5;
    optional string Key3 = 6;
}

message TSchemaEntity {
}
"""

    result = SchemaGenerator(p.parse()).generate_proto()
    assert expected == result


def test_proto():
    p = SchemaParser("""
        SCHEMA Schema {
            TABLE Table1 {
                KEY Key1;

                TABLE Table2 {
                    KEY Key2;
                    JSON_NAME test_table;

                    ROW TTable2 {
                        bool Field1;
                        string Field2;
                        repeated string Field3;
                    }
                }
            }
        }
    """)

    expected = """\
syntax = "proto2";

package NSchema;

message TTable2 {
    optional string Key1 = 1;
    optional string Key2 = 2;
    optional bool Field1 = 3;
    optional string Field2 = 4;
    repeated string Field3 = 5;
}

message TTable2UpdateRow {
    optional string Key = 1;
    optional bool Field1 = 2;
    optional string Field2 = 3;
    repeated string Field3 = 4;
}

message TTable1UpdateProto {
    optional string Type = 1;
    optional string Version = 2;
    optional string App = 3;
    optional uint64 Time = 4;
    optional string UpdateId = 5;
    optional string Key1 = 6;
    repeated TTable2UpdateRow Table2 = 7 [json_name = "test_table"];
}

message TSchemaEntity {
    oneof Row {
        TTable2 Table2 = 1;
    }
}
"""

    result = SchemaGenerator(p.parse()).generate_proto()
    assert expected == result
