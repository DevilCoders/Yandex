# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL cluster validation
"""

import re
from marshmallow.exceptions import ValidationError
from typing import Any, Dict, Sequence

from ...core.exceptions import DbaasClientError, DbaasNotImplementedError, PreconditionFailedError
from ...utils import metadb
from ...utils.config import get_postgresql_max_ha_hosts
from ...utils.validation import OneOf
from ...utils.version import Version
from .constants import DEFAULT_CONN_LIMIT
from .host_pillar import PostgresqlHostPillar
from .pillar import PostgresqlClusterPillar
from .traits import PostgresqlTasks
from .types import Database, ClusterConfig, UserWithPassword
from .utils import (
    get_max_connections_limit,
    get_postgresql_config,
    get_sum_user_conns,
    validate_database_name,
    validate_user_name,
    get_cluster_version,
)

DEFAULT_EXTENSIONS_LIST = (
    'pgvector',
    'pg_trgm',
    'uuid-ossp',
    'bloom',
    'dblink',
    'postgres_fdw',
    'clickhouse_fdw',
    'oracle_fdw',
    'orafce',
    'tablefunc',
    'pg_repack',
    'autoinc',
    'pgrowlocks',
    'hstore',
    'cube',
    'citext',
    'earthdistance',
    'btree_gist',
    'xml2',
    'isn',
    'fuzzystrmatch',
    'ltree',
    'btree_gin',
    'intarray',
    'moddatetime',
    'lo',
    'pgcrypto',
    'dict_xsyn',
    'seg',
    'unaccent',
    'dict_int',
    'postgis',
    'postgis_topology',
    'address_standardizer',
    'address_standardizer_data_us',
    'postgis_tiger_geocoder',
    'pgrouting',
    'jsquery',
    'smlar',
    'pg_stat_statements',
    'pg_stat_kcache',
    'pg_partman',
    'pg_tm_aux',
    'amcheck',
    'pg_hint_plan',
    'pgstattuple',
    'pg_buffercache',
    'timescaledb',
    'rum',
    'plv8',
    'hypopg',
    'pg_qualstats',
    'pg_cron',
)

EXTENSIONS_LIST = {
    '10': [e for e in DEFAULT_EXTENSIONS_LIST if e not in {'timescaledb', 'clickhouse_fdw'}],
    '11': list(DEFAULT_EXTENSIONS_LIST),
    '12': list(DEFAULT_EXTENSIONS_LIST),
    '13': list(DEFAULT_EXTENSIONS_LIST),
    '14': [e for e in DEFAULT_EXTENSIONS_LIST if e not in {'clickhouse_fdw'}],
}

EXTENSION_DEPENDENCIES = {
    'earthdistance': {'cube'},
    'pgrouting': {'postgis'},
    'postgis_topology': {'postgis'},
    'postgis_tiger_geocoder': {'postgis', 'fuzzystrmatch'},
    'pg_stat_kcache': {'pg_stat_statements'},
}

ALLOWED_SHARED_PRELOAD_LIBS = ['auto_explain', 'pg_hint_plan', 'timescaledb', 'pg_qualstats', 'pg_cron']

COLLATION_LIST = [
    'C',
    'aa_DJ.UTF-8',
    'aa_ER.UTF-8',
    'aa_ET.UTF-8',
    'af_ZA.UTF-8',
    'am_ET.UTF-8',
    'an_ES.UTF-8',
    'ar_AE.UTF-8',
    'ar_BH.UTF-8',
    'ar_DZ.UTF-8',
    'ar_EG.UTF-8',
    'ar_IN.UTF-8',
    'ar_IQ.UTF-8',
    'ar_JO.UTF-8',
    'ar_KW.UTF-8',
    'ar_LB.UTF-8',
    'ar_LY.UTF-8',
    'ar_MA.UTF-8',
    'ar_OM.UTF-8',
    'ar_QA.UTF-8',
    'ar_SA.UTF-8',
    'ar_SD.UTF-8',
    'ar_SY.UTF-8',
    'ar_TN.UTF-8',
    'ar_YE.UTF-8',
    'as_IN.UTF-8',
    'ast_ES.UTF-8',
    'az_AZ.UTF-8',
    'be_BY.UTF-8',
    'bem_ZM.UTF-8',
    'ber_DZ.UTF-8',
    'ber_MA.UTF-8',
    'bg_BG.UTF-8',
    'bho_IN.UTF-8',
    'bn_BD.UTF-8',
    'bn_IN.UTF-8',
    'bo_CN.UTF-8',
    'bo_IN.UTF-8',
    'br_FR.UTF-8',
    'brx_IN.UTF-8',
    'bs_BA.UTF-8',
    'byn_ER.UTF-8',
    'ca_AD.UTF-8',
    'ca_ES.UTF-8',
    'ca_FR.UTF-8',
    'ca_IT.UTF-8',
    'crh_UA.UTF-8',
    'csb_PL.UTF-8',
    'cs_CZ.UTF-8',
    'cv_RU.UTF-8',
    'cy_GB.UTF-8',
    'da_DK.UTF-8',
    'de_AT.UTF-8',
    'de_BE.UTF-8',
    'de_CH.UTF-8',
    'de_DE.UTF-8',
    'de_LI.UTF-8',
    'de_LU.UTF-8',
    'dv_MV.UTF-8',
    'dz_BT.UTF-8',
    'el_CY.UTF-8',
    'el_GR.UTF-8',
    'en_AG.UTF-8',
    'en_AU.UTF-8',
    'en_BW.UTF-8',
    'en_CA.UTF-8',
    'en_DK.UTF-8',
    'en_GB.UTF-8',
    'en_HK.UTF-8',
    'en_IE.UTF-8',
    'en_IN.UTF-8',
    'en_NG.UTF-8',
    'en_NZ.UTF-8',
    'en_PH.UTF-8',
    'en_SG.UTF-8',
    'en_US.UTF-8',
    'en_ZA.UTF-8',
    'en_ZM.UTF-8',
    'en_ZW.UTF-8',
    'eo_US.UTF-8',
    'eo.UTF-8',
    'es_AR.UTF-8',
    'es_BO.UTF-8',
    'es_CL.UTF-8',
    'es_CO.UTF-8',
    'es_CR.UTF-8',
    'es_CU.UTF-8',
    'es_DO.UTF-8',
    'es_EC.UTF-8',
    'es_ES.UTF-8',
    'es_GT.UTF-8',
    'es_HN.UTF-8',
    'es_MX.UTF-8',
    'es_NI.UTF-8',
    'es_PA.UTF-8',
    'es_PE.UTF-8',
    'es_PR.UTF-8',
    'es_PY.UTF-8',
    'es_SV.UTF-8',
    'es_US.UTF-8',
    'es_UY.UTF-8',
    'es_VE.UTF-8',
    'et_EE.UTF-8',
    'eu_ES.UTF-8',
    'eu_FR.UTF-8',
    'fa_IR.UTF-8',
    'ff_SN.UTF-8',
    'fi_FI.UTF-8',
    'fil_PH.UTF-8',
    'fo_FO.UTF-8',
    'fr_BE.UTF-8',
    'fr_CA.UTF-8',
    'fr_CH.UTF-8',
    'fr_FR.UTF-8',
    'fr_LU.UTF-8',
    'fur_IT.UTF-8',
    'fy_DE.UTF-8',
    'fy_NL.UTF-8',
    'ga_IE.UTF-8',
    'gd_GB.UTF-8',
    'gez_ER.UTF-8',
    'gez_ET.UTF-8',
    'gl_ES.UTF-8',
    'gu_IN.UTF-8',
    'gv_GB.UTF-8',
    'ha_NG.UTF-8',
    'he_IL.UTF-8',
    'hi_IN.UTF-8',
    'hne_IN.UTF-8',
    'hr_HR.UTF-8',
    'hsb_DE.UTF-8',
    'ht_HT.UTF-8',
    'hu_HU.UTF-8',
    'hy_AM.UTF-8',
    'ia.UTF-8',
    'id_ID.UTF-8',
    'ig_NG.UTF-8',
    'ik_CA.UTF-8',
    'is_IS.UTF-8',
    'it_CH.UTF-8',
    'it_IT.UTF-8',
    'iu_CA.UTF-8',
    'iw_IL.UTF-8',
    'ja_JP.UTF-8',
    'ka_GE.UTF-8',
    'kk_KZ.UTF-8',
    'kl_GL.UTF-8',
    'km_KH.UTF-8',
    'kn_IN.UTF-8',
    'kok_IN.UTF-8',
    'ko_KR.UTF-8',
    'ks_IN.UTF-8',
    'ku_TR.UTF-8',
    'kw_GB.UTF-8',
    'ky_KG.UTF-8',
    'lb_LU.UTF-8',
    'lg_UG.UTF-8',
    'li_BE.UTF-8',
    'lij_IT.UTF-8',
    'li_NL.UTF-8',
    'lo_LA.UTF-8',
    'lt_LT.UTF-8',
    'lv_LV.UTF-8',
    'mai_IN.UTF-8',
    'mg_MG.UTF-8',
    'mhr_RU.UTF-8',
    'mi_NZ.UTF-8',
    'mk_MK.UTF-8',
    'ml_IN.UTF-8',
    'mn_MN.UTF-8',
    'mr_IN.UTF-8',
    'ms_MY.UTF-8',
    'mt_MT.UTF-8',
    'my_MM.UTF-8',
    'nb_NO.UTF-8',
    'nds_DE.UTF-8',
    'nds_NL.UTF-8',
    'ne_NP.UTF-8',
    'nl_AW.UTF-8',
    'nl_BE.UTF-8',
    'nl_NL.UTF-8',
    'nn_NO.UTF-8',
    'nr_ZA.UTF-8',
    'nso_ZA.UTF-8',
    'oc_FR.UTF-8',
    'om_ET.UTF-8',
    'om_KE.UTF-8',
    'or_IN.UTF-8',
    'os_RU.UTF-8',
    'pa_IN.UTF-8',
    'pap_AN.UTF-8',
    'pa_PK.UTF-8',
    'pl_PL.UTF-8',
    'ps_AF.UTF-8',
    'pt_BR.UTF-8',
    'pt_PT.UTF-8',
    'ro_RO.UTF-8',
    'ru_RU.UTF-8',
    'ru_UA.UTF-8',
    'rw_RW.UTF-8',
    'sa_IN.UTF-8',
    'sc_IT.UTF-8',
    'sd_IN.UTF-8',
    'sd_PK.UTF-8',
    'se_NO.UTF-8',
    'shs_CA.UTF-8',
    'sid_ET.UTF-8',
    'si_LK.UTF-8',
    'sk_SK.UTF-8',
    'sl_SI.UTF-8',
    'so_DJ.UTF-8',
    'so_ET.UTF-8',
    'so_KE.UTF-8',
    'so_SO.UTF-8',
    'sq_AL.UTF-8',
    'sq_MK.UTF-8',
    'sr_ME.UTF-8',
    'sr_RS.UTF-8',
    'ss_ZA.UTF-8',
    'st_ZA.UTF-8',
    'sv_FI.UTF-8',
    'sv_SE.UTF-8',
    'sw_KE.UTF-8',
    'sw_TZ.UTF-8',
    'ta_IN.UTF-8',
    'ta_LK.UTF-8',
    'te_IN.UTF-8',
    'tg_TJ.UTF-8',
    'th_TH.UTF-8',
    'ti_ER.UTF-8',
    'ti_ET.UTF-8',
    'tig_ER.UTF-8',
    'tk_TM.UTF-8',
    'tl_PH.UTF-8',
    'tn_ZA.UTF-8',
    'tr_CY.UTF-8',
    'tr_TR.UTF-8',
    'ts_ZA.UTF-8',
    'tt_RU.UTF-8',
    'ug_CN.UTF-8',
    'uk_UA.UTF-8',
    'unm_US.UTF-8',
    'ur_IN.UTF-8',
    'ur_PK.UTF-8',
    'uz_UZ.UTF-8',
    've_ZA.UTF-8',
    'vi_VN.UTF-8',
    'wa_BE.UTF-8',
    'wae_CH.UTF-8',
    'wal_ET.UTF-8',
    'wo_SN.UTF-8',
    'xh_ZA.UTF-8',
    'yi_US.UTF-8',
    'yo_NG.UTF-8',
    'yue_HK.UTF-8',
    'zh_CN.UTF-8',
    'zh_HK.UTF-8',
    'zh_SG.UTF-8',
    'zh_TW.UTF-8',
    'zu_ZA.UTF-8',
]

COLLATE_VALIDATOR = OneOf(COLLATION_LIST)

UPGRADE_TASKS = {
    '11-1c': PostgresqlTasks.upgrade11_1c,
    '11': PostgresqlTasks.upgrade11,
    '12': PostgresqlTasks.upgrade12,
    '13': PostgresqlTasks.upgrade13,
    '14': PostgresqlTasks.upgrade14,
}

UPGRADE_PATHS = {
    '11-1c': {
        'from': ['10-1c'],
    },
    '11': {
        'from': ['10'],
    },
    '12': {
        'from': ['11'],
    },
    '13': {
        'from': ['12'],
    },
    '14': {
        'from': ['13'],
    },
}


def validate_version_upgrade(version_raw: str, new_version_raw: str):
    """
    Check if upgrade from version_raw to new_version_raw is possible for cluster
    """
    upgrade_not_supported_msg = f'Upgrade from {version_raw} to {new_version_raw} is not supported'
    version, new_version = Version.load(version_raw), Version.load(new_version_raw)
    if version.edition != new_version.edition:
        raise DbaasNotImplementedError(upgrade_not_supported_msg)
    if version > new_version:
        raise DbaasClientError('Version downgrade detected')
    try:
        if version_raw not in UPGRADE_PATHS[new_version_raw]['from']:
            raise DbaasNotImplementedError(upgrade_not_supported_msg)
    except KeyError:
        raise DbaasNotImplementedError(upgrade_not_supported_msg)


# when we create another type
# make UserT = Union[UserWithPassword, NewUserType]
UserT = UserWithPassword


def validate_database_owner(database: Database, users: Sequence[UserT]) -> None:
    """
    Validate that owner present for our database
    """
    if database.owner not in {u.name for u in users}:
        raise DbaasClientError(message='Database owner \'{name}\' not found'.format(name=database.owner))


def validate_database(database: Database, users: Sequence[UserT], cluster_config: ClusterConfig) -> None:
    """
    Validate database
    """
    validate_database_owner(database, users)
    validate_database_name(database.name)
    validate_extensions(database.extensions, str(cluster_config.version.major), cluster_config.db_options)


def validate_user_connect_dbs(user: UserT, databases: Sequence[Database]) -> None:
    """
    Validate that all items in connect_dbs exists in database
    """
    existed_db_names = {db.name for db in databases}
    for db_name in user.connect_dbs:
        if db_name not in existed_db_names:
            # throw client error instead of DatabaseNotExistError,
            # cause it invalid-arguments in create (restore) cluster
            # error: "Database 'nonexistent' does not exist"
            raise DbaasClientError(
                "The specified permission for user '{user_name}' is invalid. "
                "The database '{db_name}' does not exist.".format(user_name=user.name, db_name=db_name)
            )


def validate_user(user: UserT, databases: Sequence[Database]) -> None:
    """
    Validate user
    """
    validate_user_name(user.name)
    validate_user_connect_dbs(user, databases)


def validate_user_conns(user: UserT, cluster_pillar: PostgresqlClusterPillar, flavor: dict) -> None:
    """
    Check if adding user to existing users will exceed max_connections
    """

    max_connections = (
        cluster_pillar.config.max_connections
        if cluster_pillar.config.max_connections is not None
        else get_max_connections_limit(flavor)
    )

    connections = get_sum_user_conns([u for u in cluster_pillar.pgusers.get_users() if u.name != user.name])

    user_conns = user.conn_limit if user.conn_limit is not None else DEFAULT_CONN_LIMIT  # type: int

    if connections + user_conns > max_connections:
        raise PreconditionFailedError(
            "'conn_limit' is too high for user '{user}'. "
            "Consider increasing 'max_connections'".format(user=user.name)
        )


def validate_version_and_ha(cluster, host_spec, repl_source):
    """
    Get host spec, cluster, replication_source and validate version and HA
    """
    if 'config_spec' in host_spec:
        config = get_postgresql_config(host_spec)
        config_options = config.get_config()
        config_version = str(config.version)
        cluster_version = get_cluster_version(cluster['cid']).to_string()
        if config_version != cluster_version:
            raise DbaasClientError('host config version not equal cluster version')
        if host_spec.get('replication_source', repl_source) is None and config_options:
            raise DbaasClientError('host config apply only on non HA host')


def validate_postgresql_host_config(fqdn, host_spec, cluster):
    """
    Get host spec and cluster and validate changes
    """
    changes = False
    host_pillar = PostgresqlHostPillar(metadb.get_fqdn_pillar(fqdn=fqdn))
    config_options = get_postgresql_config(host_spec).get_config()
    pillar_config = host_pillar.get_config() or dict()
    validate_version_and_ha(cluster, host_spec, host_pillar.get_repl_source())
    for option in config_options:
        if config_options.get(option) != pillar_config.get(option):
            changes = True
    return changes


def validate_priority(fqdn, host_spec):
    """
    Get host spec and pillar (optional) and validate priority changes
    """
    host_pillar = PostgresqlHostPillar(metadb.get_fqdn_pillar(fqdn=fqdn))
    if host_spec.get('priority') == host_pillar.get_priority():
        return False
    return True


def validate_extensions(extensions: Sequence[str], pg_major_version: str, config_pillar: Dict[str, Any]):
    """
    Validation extensions, its dependencies and shared_preload_libraries
    """
    if 'rum' in extensions and 'smlar' in extensions:
        raise DbaasClientError("Cannot use 'smlar' and 'rum' at the same time, because they use a common '%' operator")

    one_of_validate = OneOf(EXTENSIONS_LIST[pg_major_version])
    user_shared_preload_libraries = config_pillar.get('user_shared_preload_libraries', [])
    for extension in extensions:
        try:
            one_of_validate(extension)
        except Exception as e:
            raise DbaasClientError(str(e))

        if extension in ALLOWED_SHARED_PRELOAD_LIBS and extension not in user_shared_preload_libraries:
            raise DbaasClientError(f"The specified extension '{extension}' is not present in shared_preload_libraries.")

        if extension not in EXTENSION_DEPENDENCIES:
            continue

        if not EXTENSION_DEPENDENCIES[extension].issubset(extensions):
            raise DbaasClientError(
                f"The specified extension '{extension}' requires {', '.join(EXTENSION_DEPENDENCIES[extension])}"
            )


def validate_databases(databases: Sequence[Database]):
    if len([db for db in databases if 'pg_cron' in db.extensions]) > 1:
        raise DbaasClientError("Extension 'pg_cron' should only be in one database")


def validate_search_path(path):
    if not path:
        return
    if re.search("[\r\n\0\b\t\x1A]", path):
        raise ValidationError('"search_path" contains invalid characters')
    for p in path.split(','):
        p = p.strip()
        # plain identifier
        if re.match(r'[\w$]+', p, re.U):
            continue
        # quoted identifier
        if re.match(r'"[^"]*"', p):
            continue
        raise ValidationError('"search_path" part {} is invalid'.format(p))


def validate_ha_host_count(hosts_with_repl_source, host_count):
    for host in hosts_with_repl_source:
        if hosts_with_repl_source.get(host, None):
            host_count -= 1
    if host_count > get_postgresql_max_ha_hosts():
        raise DbaasClientError("PostgreSQL cluster allows at most 17 HA hosts")


def validate_pg_qualstats_sample_rate(value):
    if 0.0 <= value <= 1.0 or value == -1:
        return True
    raise ValidationError("pg_qualstats.sample_rate should be between 0.0 and 1.0 or -1")
