# -*- encoding: utf-8 -*-
from __future__ import unicode_literals


def test_levels(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    levels = fias.get_levels()

    assert len(levels) == 14


def test_get_address_info(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_addr_info(
        base_ids=['2ce195fb-fc4a-4a3a-9492-8be5da769cd6'],
        parent_ids=['8dea00e3-9aab-4d8e-887c-ef2aaa546456'],
        with_archived=True,
        levels=[7],
        name='Красно'
    )

    assert results
    assert len(results) == 3


def test_get_house_info(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_house_info(
        base_ids=['f58acc35-9f87-40d2-a8f6-014cec76fc42'],
        parent_ids=['2ce195fb-fc4a-4a3a-9492-8be5da769cd6'],
        with_archived=True,
        num='35'
    )

    assert results
    assert len(results) == 2


def test_get_stead_info(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_stead_info(
        base_ids=['fba1f561-d91d-445f-8639-e6e2dc461514'],
        parent_ids=['7ba3a440-69fd-4b37-85e3-cdbdddd08229'],
        with_archived=True,
    )

    assert results
    assert len(results) == 7


def test_get_doc_info(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_doc_info(
        base_ids=['ffffff6b-86bd-4f86-8f62-c302e8d401ad'],
    )

    assert results
    assert len(results) == 1

def test_get_room_info(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_room_info(
        base_ids=['0000000b-419e-41be-875b-583ff1655345'],
        parent_ids=['bdc0c198-0e2b-4298-b76c-ab6d8b57255c'],
        num='2',
        num_flat='59'
    )

    assert results
    assert len(results) == 1


def test_get_live_status(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_live()

    assert results
    assert len(results) == 2


def test_get_division(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_division()

    assert results
    assert len(results) == 3

def test_get_type_doc(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_type_doc()

    assert results
    assert len(results) == 26

def test_get_status_operational(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_operational()

    assert results
    assert len(results) == 20

def test_get_status_current(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_current()

    assert results
    assert len(results) == 100

def test_get_status_center(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_center()

    assert results
    assert len(results) == 5

def test_get_status_actual(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_actual()

    assert results
    assert len(results) == 2

def test_get_type_flat(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_type_flat()

    assert results
    assert len(results) == 14


def test_get_type_room(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_type_room()

    assert results
    assert len(results) == 3


def test_get_status_estate(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_estate()

    assert results
    assert len(results) == 11


def test_get_status_struct(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_struct()

    assert results
    assert len(results) == 4


def test_get_status_state(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    fias = refs.get_ref_fias()

    results = fias.get_status_state()

    assert results
    assert len(results) == 43


