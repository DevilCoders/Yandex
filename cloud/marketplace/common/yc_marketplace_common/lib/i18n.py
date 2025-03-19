import time
from typing import Optional

from natsort import natsorted
from flask import request

from cloud.marketplace.common.yc_marketplace_common.db.models import i18n_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.models.i18n import BulkI18nCreatePublic
from cloud.marketplace.common.yc_marketplace_common.models.i18n import I18n as I18nScheme
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import logging
from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlInsertValues
from yc_common.misc import timestamp

log = logging.get_logger(__name__)


# i18n не транзакционна, тк в этом месте не нужна консистентность


def _check_str(s):
    return type(s) is str and s.startswith(I18nScheme.PREFIX)


def _check_simple(o):
    return type(o) in {dict, set, list, tuple}


def _get_locale():
    return request.accept_languages.best_match(I18nScheme.LANGUAGES)


class I18n:
    @staticmethod
    @retry_idempotent_kikimr_errors
    def get(id: str, lang=None, fallback=I18nScheme.FALLBACK_LINE):
        translations = marketplace_db().with_table_scope(i18n_table). \
            select("SELECT " + I18nScheme.db_fields() + " FROM $table WHERE id = ?", id)
        if translations is None:
            log.error("Missing translation for %s" % id)
            return ""

        if lang is None:
            lang = _get_locale()

        lang_text = {translation["i18n.lang"]: translation["i18n.text"] for translation in translations}

        if lang in lang_text:
            return lang_text[lang]

        for key in fallback:
            if key in lang_text:
                log.warn("Missing language %s for key %s, use fallback %s" % (lang, id, key))
                return lang_text[key]

        log.error("Missing translation for key %s for lang %s   " % (id, lang))
        return ""

    @staticmethod
    @retry_idempotent_kikimr_errors
    def set(id: str, translations: dict):
        if not id.startswith(I18nScheme.PREFIX):
            id = I18nScheme.PREFIX + id

        time = timestamp()
        values = SqlInsertValues(["id", "lang", "text", "created_at"],
                                 [[id, lang, translations[lang], time] for lang in translations])
        marketplace_db().with_table_scope(i18n_table).query("UPSERT INTO $table ? ", values, commit=True)

        return id

    @staticmethod
    @retry_idempotent_kikimr_errors
    @mkt_transaction()
    def rpc_create_bulk(translations: BulkI18nCreatePublic, *, tx):
        time = timestamp()
        raw_values = []
        for translation in translations.translations:
            tid = translation.get("_id", generate_id())
            if not tid.startswith(I18nScheme.PREFIX):
                tid = I18nScheme.PREFIX + tid
            translation["_id"] = tid
            for lang in I18nScheme.LANGUAGES:
                text = translation.get(lang, "")
                raw_values.append([
                    tid, lang, text, time,
                ])
                translation[lang] = text

        sql_values = SqlInsertValues(["id", "lang", "text", "created_at"], raw_values)
        tx.with_table_scope(i18n_table).query("UPSERT INTO $table ? ", sql_values)
        return translations

    @staticmethod
    def _get_keys(obj):
        if not _check_simple(obj):
            return [obj] if _check_str(obj) else []
        keys = []
        if type(obj) in {dict, set}:
            for key in obj:
                if _check_simple(obj[key]):
                    keys += I18n._get_keys(obj[key])
                if _check_str(obj[key]):
                    keys.append(obj[key])

        if type(obj) in {list, tuple}:
            for item in obj:
                if _check_simple(item):
                    keys += I18n._get_keys(item)
                if _check_str(item):
                    keys.append(item)

        return keys

    @staticmethod
    def _get_batch(keys, lang=None, fallback=I18nScheme.FALLBACK_LINE):  # TODO max batch size less 200
        if lang is None:
            lang = _get_locale()

        # tx = marketplace_db().with_table_scope(i18n_table)
        # query = """
        #     DECLARE $ydb_ids as "List<Struct<id: Utf8>>";
        #     SELECT {}
        #     FROM as_table($ydb_ids) as ids
        #     INNER JOIN $table as i18n
        #     ON i18n.id = ids.id;
        # """.format(I18nScheme.db_fields("i18n"))
        # p_query = tx.prepare_query(query)
        # translations = tx.select(p_query, {"$ydb_ids": [{"id": id} for id in keys]})
        translations = marketplace_db().with_table_scope(i18n_table).select("SELECT * FROM $table WHERE ?",
                                                                            SqlIn("id", keys))

        if translations is None:
            log.error("Missing translation for keys %s" % keys)
            return {}

        lang_keys = {}
        for translation in translations:
            # if translation["i18n.id"] not in lang_keys:
            #     lang_keys[translation["i18n.id"]] = {}
            # lang_keys[translation["i18n.id"]][translation["i18n.lang"]] = translation["i18n.text"]
            if translation["id"] not in lang_keys:
                lang_keys[translation["id"]] = {}
            lang_keys[translation["id"]][translation["lang"]] = translation["text"]

        ret = {}
        for key in keys:
            if key not in lang_keys:
                log.error("Missing translation for key %s" % key)
                ret[key] = ""
            elif lang not in lang_keys[key]:
                for f_lang in fallback:
                    if f_lang in lang_keys[key]:
                        log.warn("Missing language %s for key %s, use fallback %s" % (lang, key, f_lang))
                        ret[key] = lang_keys[key][f_lang]
                if key not in ret:
                    log.error("Missing translation for key %s for lang %s   " % (key, lang))
                    ret[key] = ""
            else:
                ret[key] = lang_keys[key][lang]
        return ret

    @staticmethod
    def _fill_items(obj, translations, to_sort: Optional[dict] = None):
        if not _check_simple(obj):
            return translations.get(obj, "") if _check_str(obj) else obj

        if isinstance(obj, dict):
            ret = {}
            for key in obj:
                res = I18n._fill_items(obj[key], translations, to_sort=to_sort)
                if to_sort is not None and key in to_sort and isinstance(res, list):
                    res = natsorted(res, key=to_sort[key])
                ret[key] = res
            return ret

        if isinstance(obj, (list, tuple, set)):
            ret = []
            for item in obj:
                ret.append(I18n._fill_items(item, translations, to_sort=to_sort))
            return ret

    @staticmethod
    def traverse_simple(obj, lang=None, fallback=I18nScheme.FALLBACK_LINE, to_sort=None):
        start_time = time.monotonic()
        keys = I18n._get_keys(obj)
        translations = I18n._get_batch(keys, lang, fallback)
        translated = I18n._fill_items(obj, translations, to_sort)

        log.debug("Method traverse_simple time: %s ms" % (time.monotonic() - start_time))

        return translated
