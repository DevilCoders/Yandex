import pytest

import schematics.exceptions as schematics_exceptions

import yc_common.models as common_models

import yc_common.clients.models.labels as yc_labels


def test_validate_labels_success():
    class M(common_models.Model):
        labels = yc_labels.LabelsType(required=True)

    M({"labels": {"key1": "value1", "key2": "value2"}}, validate=True)


def test_validate_labels_bad_key():
    class M(common_models.Model):
        labels = yc_labels.LabelsType(required=True)

    with pytest.raises(schematics_exceptions.DataError) as ex:
        M({"labels": {"1key": "value1", "key2": "value2"}}, validate=True)
    assert ex.value.errors["labels"][0].summary.startswith("Invalid label key")


def test_validate_labels_bad_value():
    class M(common_models.Model):
        labels = yc_labels.LabelsType(required=True)

    with pytest.raises(schematics_exceptions.DataError) as ex:
        M({"labels": {"key": "value1:", "key2": "value2"}}, validate=True)
    assert ex.value.errors["labels"][0].summary.startswith("Invalid label value")


def test_validate_labels_max_count():
    class M(common_models.Model):
        labels = yc_labels.LabelsType(required=True)

    with pytest.raises(schematics_exceptions.DataError) as ex:
        M({"labels": {"key{}".format(n): "value" for n in range(100)}}, validate=True)
    assert ex.value.errors["labels"][0].summary.startswith("Too many labels")
