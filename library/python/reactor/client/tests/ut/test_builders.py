import reactor_client.reactor_objects as r_objs
from reactor_client.reaction_builders import (
    make_input_node_from_dict,
    make_output_node_from_dict,
    make_metadata_from_val,
    make_metadata_from_val_with_hints
)


def test_make_input_node_produces_expected_object():
    dict_ = {
        "0": 1
    }

    expected_obj = r_objs.InputsValueNode(
        nodes={
            "0": r_objs.InputsValueElement(value=r_objs.InputsValueRef(const_value=r_objs.InputsValueConst(
                generic_value=r_objs.Metadata(
                    type_="/yandex.reactor.artifact.IntArtifactValueProto",
                    dict_obj={"value": "1"}
                )
            )))
        }
    )

    assert make_input_node_from_dict(dict_) == expected_obj


def test_make_output_node_produces_expected_object():
    dict_ = {
        "0": r_objs.Expression("return 1;")
    }

    expected_obj = r_objs.OutputsValueNode(
        nodes={
            "0": r_objs.OutputsValueElement(value=r_objs.OutputsValueRef(
                expression=r_objs.Expression("return 1;")
            ))
        }
    )

    assert make_output_node_from_dict(dict_) == expected_obj


def test_make_metadata_from_val():
    expected_int_obj = r_objs.Metadata(
        type_="/yandex.reactor.artifact.IntArtifactValueProto",
        dict_obj={"value": "1"}
    )

    assert make_metadata_from_val(1) == expected_int_obj

    expected_bool_obj = r_objs.Metadata(
        type_="/yandex.reactor.artifact.BoolArtifactValueProto",
        dict_obj={"value": True}
    )

    assert make_metadata_from_val(True) == expected_bool_obj

    expected_string_obj = r_objs.Metadata(
        type_="/yandex.reactor.artifact.StringArtifactValueProto",
        dict_obj={"value": "some_str_val"}
    )

    assert make_metadata_from_val("some_str_val") == expected_string_obj


def test_make_metadata_from_val_with_hints():
    expected_int_obj = r_objs.Metadata(
        type_="/yandex.reactor.artifact.IntArtifactValueProto",
        dict_obj={"value": 1}
    )
    assert make_metadata_from_val_with_hints(1, r_objs.VariableTypes.INT) == expected_int_obj

    expected_float_obj = r_objs.Metadata(
        type_="/yandex.reactor.artifact.FloatArtifactValueProto",
        dict_obj={"value": 4.2}
    )
    assert make_metadata_from_val_with_hints(
        4.2, r_objs.VariableTypes.FLOAT
    ) == expected_float_obj

    expected_list_float = r_objs.Metadata(
        type_="/yandex.reactor.artifact.FloatListArtifactValueProto",
        dict_obj={"values": [1.0, 4.2]}
    )
    assert make_metadata_from_val_with_hints(
        [1, 4.2], r_objs.VariableTypes.LIST_FLOAT
    ) == expected_list_float
