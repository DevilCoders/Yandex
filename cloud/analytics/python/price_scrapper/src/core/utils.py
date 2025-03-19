from typing import List, Union, Any, Collection, Iterable

from pandas import set_option as pd_set_option

from core.tariff import *


def compute_tariff(tariffs: List[Union[Tariff, ComputeTariff]],
                   service: Service,
                   commitment: Commitment = Commitment.ON_DEMAND) -> Tariff:
    for_service = [t for t in tariffs if t.service == service]
    if len(for_service) == 0:
        raise Exception(f"No tariff for service = {service}")
    if len(for_service) == 1:
        return for_service[0]
    else:
        for_commitment = [t for t in for_service if t.commitment == commitment]
        if len(for_commitment) == 0:
            raise Exception(f"No tariff for service = {service} and commitment = {commitment}")
        return for_commitment[0]


def storage_tariff(tariffs: List[Union[Tariff, StorageTariff]], stt: StorageType, serv: StorageServiceType):
    filtered = [t for t in tariffs if t.storage_type == stt and t.service == serv]
    if len(filtered) == 0:
        raise Exception("No tariff with such params")
    if len(filtered) > 1:
        raise Exception("Mulptiple tariffs with such params")
    return filtered[0]


def assert_vat(tariff_menu: Any, vat_expected: bool):
    for t in tariff_menu.__dict__.values():
        if not hasattr(t, "with_vat"):
            continue
        if t.with_vat != vat_expected:
            raise Exception(f"Tariff [{t}] VAT state: "
                            f"expected = {vat_expected}, but actual is {t.with_vat}")


def assert_operating_system(tariff_menu: Any, expected: OperSys):
    for t in tariff_menu.__dict__.values():
        if not hasattr(t, "oper_sys"):
            continue
        if not isinstance(t.oper_sys, OperSys):
            continue
        if t.oper_sys != expected:
            raise Exception(f"Tariff's [{t}] operating system is expected to be {expected}, "
                            f"but actual is {t.oper_sys}")


def assert_company_name(tariff_menu: Any, expected: str):
    for t in tariff_menu.__dict__.values():
        if not hasattr(t, "company"):
            continue
        if t.company != expected:
            raise Exception(f"Tariff's [{t}] company is expected to be {expected}, "
                            f"but actual is {t.company}")


def assert_no_vat(tariff_menu: Any):
    assert_vat(tariff_menu, False)


def assert_linux(tariff_menu: Any):
    assert_operating_system(tariff_menu, OperSys.Linux)


def assert_windows(tariff_menu: Any):
    assert_operating_system(tariff_menu, OperSys.Windows)


def assert_storage_pricing_units(menu: StorageTariffMenu):
    def _assert(condition: bool, msg: str):
        if not condition:
            raise Exception(msg)

    _assert(menu.cold_per_space.pricing_unit == "Gb", "Gb pricing unit is expected")
    _assert(menu.cold_read.pricing_unit == "10000 requests", "Per 10000 requests pricing unit is expected")
    _assert(menu.cold_write.pricing_unit == "1000 requests", "Per 1000 requests pricing unit is expected")


def pandas_display_all():
    pd_set_option('display.max_rows', 500)
    pd_set_option('display.max_columns', 500)
    pd_set_option('display.width', 1000)


def first(obj: Any):
    if hasattr(obj, "__getitem__"):
        if len(obj) >= 1:
            return obj[0]
        else:
            raise ValueError(f"Fail to get first: object is empty or not a collection: {obj}")


def json_attr(obj, path: str) -> Any:
    name1 = path.replace(' ', '').split('->')[0]
    name2 = path.replace(' ', '').split('->')[1]
    if name1 in obj:
        if name2 in obj[name1]:
            return obj[name1][name2]
        else:
            return None
    else:
        return None


def get_compute_tariffs(menu: Any) -> List[ComputeTariff]:
    if menu is None:
        return []
    return [t for t in menu.__dict__.values() if hasattr(t, "oper_sys") and isinstance(t, ComputeTariff)]
