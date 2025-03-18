import pytest
import sform


@pytest.fixture
def form_cls():
    class FieldSetF(sform.SForm):
        iner_int = sform.IntegerField()

    class Form(sform.SForm):
        int_field = sform.IntegerField()
        field_set = sform.FieldsetField(FieldSetF)
        field_set_grid = sform.GridField(sform.FieldsetField(FieldSetF))
        int_grid = sform.GridField(sform.IntegerField())

    return Form


form_data = {
    'int_field': 1,
    'field_set': {
        'iner_int': 2,
    },
    'field_set_grid': [
        {'iner_int': 3},
        {'iner_int': 4},
    ],
    'int_grid': [5, 6, 7],
}
