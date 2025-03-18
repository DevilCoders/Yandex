# encoding=utf8
import antiadblock.analytics_service.service.modules as modules

MODULES = modules.__all__


def test_check_key_uniq():
    """
    Проверка уникальности ключа проверки в модулях. Если они пересекаются то тесты могут влиять на результаты друг друга
    """
    keys_by_module = {}
    for module in MODULES:
        keys_by_module[module] = getattr(modules, module).CHECK_KEYS

    for module in MODULES:
        assert set(keys_by_module[module]).intersection(set(sum([keys for mod, keys in keys_by_module.items() if mod != module], []))) == set()
