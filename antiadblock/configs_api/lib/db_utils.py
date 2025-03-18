# coding=utf-8
from copy import deepcopy

from sqlalchemy.orm import joinedload, load_only, contains_eager
from sqlalchemy.sql import text, and_

from antiadblock.configs_api.lib.db import db, ServiceStatus, Service, Config, ConfigMark, ConfigStatus
from antiadblock.configs_api.lib.const import ROOT_LABEL_ID


def get_config_by_status(service_id, status, label_id=None):
    query = Config.query.join(ConfigMark).options(joinedload(Config.statuses))
    if label_id:
        query = query.filter(Config.label_id == label_id)
    else:
        query = query.filter(Config.service_id == service_id)
    return query.filter(ConfigMark.status == status).first_or_404()


def get_label_config_by_id(config_id, label_id):
    return Config.query.filter(and_(Config.id == config_id, Config.label_id == label_id)).first()


def get_service_id_for_label(label_id):
    config = Config.query.options(load_only("service_id")).filter_by(label_id=label_id).first()
    if config:
        return config.service_id
    return None


def is_label_exist(label_id):
    return db.session.query(db.session.query(Config).filter(Config.label_id == label_id).exists()).scalar()


def get_exp_ids_from_parent_labels(label_id, parent_label_id):
    exp_ids = set()
    while label_id != ROOT_LABEL_ID:
        configs = Config.query.filter_by(label_id=parent_label_id).all()
        if not configs:
            break

        exp_ids |= {config.exp_id for config in configs if config.exp_id}
        label_id, parent_label_id = parent_label_id, configs[0].parent_label_id

    return sorted(exp_ids)


def get_data_from_parent_configs(label_id, parent_label_id, new_config, status=ConfigStatus.ACTIVE, exp_id=None, with_defaults=False):

    old_parent_data = {}
    new_parent_data = {}
    parent_configs = []
    while label_id != ROOT_LABEL_ID:
        if exp_id:
            # try get parent config with exp_id if exist
            p_config = Config.query.filter_by(label_id=parent_label_id, exp_id=exp_id).first()
            # else get parent config with status 'active'
            if p_config is None:
                p_config = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.label_id == parent_label_id, ConfigMark.status == ConfigStatus.ACTIVE).first()
        else:
            p_config = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.label_id == parent_label_id, ConfigMark.status == status).first()
        if p_config is None:
            break
        parent_configs.append(p_config)
        parent_label_id = p_config.parent_label_id
        label_id = p_config.label_id

    for p_config in parent_configs[::-1]:
        old_parent_data = merge_data(old_parent_data, p_config.data, p_config.data_settings)
        if new_config:
            tmp_config = new_config if new_config.label_id == p_config.label_id else p_config
            new_parent_data = merge_data(new_parent_data, tmp_config.data, tmp_config.data_settings)

    if with_defaults:
        from antiadblock.configs_api.lib.validation.template import TEMPLATE
        old_parent_data = TEMPLATE.fill_defaults(old_parent_data)
        if new_config:
            new_parent_data = TEMPLATE.fill_defaults(new_parent_data)

    return old_parent_data, new_parent_data


def get_final_configs(label_id, status=ConfigStatus.ACTIVE, exp_id=None):
    # вытаскиваем все конечные конфиги с выбранным статусом или экспериментом
    # для поддерева, в котором label_id является родителем
    result = {"configs": {}}
    label_ids = [label_id]
    while label_ids:
        new_label_ids = []
        for l_id in label_ids:
            if exp_id:
                configs = Config.query.filter_by(parent_label_id=l_id, exp_id=exp_id).all()
                if not configs:
                    configs = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.parent_label_id == l_id, ConfigMark.status == ConfigStatus.ACTIVE).all()
            else:
                configs = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.parent_label_id == l_id, ConfigMark.status == status).all()
            for config in configs:
                # конфиг без service_id или хотя бы с одним ребенком - родительский
                if not config.service_id or Config.query.filter_by(parent_label_id=config.label_id).first() is not None:
                    new_label_ids.append(config.label_id)
                else:
                    result["configs"][config.service_id] = config

        label_ids = new_label_ids[:]
    return result


def merge_data(parent_data, config_data, config_data_settings):
    merged_data = deepcopy(parent_data)
    merged_data.update(config_data)
    # unset fields
    for key in config_data_settings:
        if config_data_settings[key].get("UNSET", False) and key in merged_data:
            del merged_data[key]
    return merged_data


def get_active_config(service_id=None, label_id=None):
    return get_config_by_status(service_id, ConfigStatus.ACTIVE, label_id)


def is_service_active(service_id):
    service = Service.query.get_or_404(service_id)
    return not service.status == ServiceStatus.INACTIVE


def create_lock(lock_id_str, timeout_seconds=0):
    """
    Создаёт блокировку в базе данных с указанным таймаутом (если ноль, то таймаут не применяется),
    если таймаут истекает,то транзакция откатывается и БД выбрасывает исключение. Блокировка берётся
    на всё время транзакции.
    ВАЖНО! Метод должен вызываться в рамках существующей транзакции
    ВАЖНО! Метод меняет значение lock_timeout для транзакции, поэтому если оно выставлялось ранне, то будет перетёрто

    :param lock_id_str: айди блокировки, которую надо взять. Крайне не рекомендуется использовать динамические значения,
    потому что индификатор сохраняется в базу данных навсегда
    :param timeout_seconds: таймаут блокировки в секундах, указывает, сколько ожидать блокировку, если она уже взята
    """
    db.engine.execute("set local lock_timeout to '{}s'".format(int(timeout_seconds)))
    ensure_id_query = text("""insert into db_locks values(:id) on conflict on constraint db_locks_pkey do nothing""")
    lock_query = text("""select * from db_locks where id = :id for update""")
    db.engine.execute(ensure_id_query, id=lock_id_str)
    db.engine.execute(lock_query, id=lock_id_str)
