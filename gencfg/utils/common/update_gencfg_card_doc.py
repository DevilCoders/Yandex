#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import xmlrpclib
from getpass import getuser, getpass

import gencfg
from core.db import CURDB
from core.card.updater import CardUpdater
from core.card.node import TMdDoc
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from config import GENCFG_GROUP_CARD_WIKI_URL, SCHEME_LEAF_DOC_FILE

ACTIONS = ["show", "update"]


def get_parser():
    parser = ArgumentParserExt(description="Update group card wiki")
    parser.add_argument("-a", "--action", required=True,
                        choices=ACTIONS,
                        help="Obligatory. Choose between show doc and update wiki")
    parser.add_argument("--sections", type=core.argparse.types.comma_list, default=None,
                        help="Optional. Comma list of sections to show (incompatable with <-a update> options")
    parser.add_argument("-u", "--user", type=str, default=getuser(),
                        help="Optional. Custom user name for update (%s by default)" % getuser())

    return parser


def generate_table_line(d, md_doc):
    web_name = d["display_name"]
    scheme_name = ".".join(d["path"])

    description = md_doc.get_doc("group.yaml", scheme_name, TMdDoc.EFormat.MD)
    if description is None:
        description = d["description"] if d["description"] else d["display_name"]

    if len(d.get("children", [])) > 0:
        extra = generate_table(d, md_doc)
    else:
        extra = ""

    return "|| {{a name=\"%s\"}} %s | %s | %%%%(markdown) %s %%%% %s ||" % (
    scheme_name, web_name, scheme_name, description, extra)


def generate_table(d, md_doc):
    table_lines = ["|| Web Name | Scheme Name | Description ||"] + map(lambda x: generate_table_line(x, md_doc),
                                                                       d["children"])

    table_body = """#|
%s
|#""" % ("\n".join(table_lines))
    return table_body


def generate_section(d, md_doc):
    section_name = d["display_name"]

    section_description = md_doc.get_doc("group.yaml", ".".join(d["path"]), TMdDoc.EFormat.MD)
    if section_description is None:
        section_description = d["description"]

    table_body = generate_table(d, md_doc)

    result = """=== %s ===
%%%%(markdown) %s %%%%

%s""" % (section_name, section_description, table_body)

    return result


def generate_doc():
    md_doc_file = os.path.join(CURDB.SCHEMES_DIR, SCHEME_LEAF_DOC_FILE)
    md_doc = TMdDoc(md_doc_file)

    group = CURDB.groups.get_group('MSK_RESERVED')
    full_card_info = CardUpdater().get_group_card_info(group)

    head = """== Карточка группы в web-интерфейсе ==
Карточка группы представляет собой древовидную структуру с атрибутами группы. Карточка представляет собой текстовый yaml-файл и хранятся в нашей базе. У карточки есть схема, описываемая yaml-файлом специального формата, тоже хранящегося в нашей базе **((https://git.qe-infra.yandex-team.ru/projects/NANNY/repos/gencfg.db/browse/schemes/group.yaml здесь))**. Большинство действий в web-интерфейсе - это манипуляции с карточкой группы, многие из которых могут быть выполнены скриптами из консоли или даже редактированием карточки группы вручную с последующим коммитом.

В web-интерфейсе карточка отображается почти один в один с тем, как хранится в нашей базе. Текущую карточку для группы можно увидеть **((https://gencfg.yandex-team.ru/trunk/groups/MSK_RESERVED#card здесь))** (на примере карточки для MSK_RESERVED). Ее поля в одной из секций в SETTINGS на странице группы. Все редактируемые поля разбиты на категории, чтобы было легче ориентироваться. В редактировании полей в группе есть кнопка "Load params from template group", которая позволяет загрузить все (или желаемые поля) из карточки другой группы (это нужно на случай, если вы делаете группу, аналогичную какой-то другой и не хотите вручную вбивать все поля).

== Основные поля карточки группы ==
В каждой группе настроек есть поля как важные, так и не очень, поэтому мы подробно распишем только самые важные с указанием того, как, кем и где они используются. Поля схемы представлены в виде таблицы со следующими параметрами:
* **Web name** - имя параметра в web-интерфейсе;
* **Scheme name** - имя параметра в схеме;
* **Description** - более подробное описание параметра со ссылками на то, где он используется.
"""

    sections_list = full_card_info["children"]
    if options.sections is not None:
        sections_list = filter(lambda x: x["anchor_name"] in options.sections, sections_list)

    sections_result = "\n".join(map(lambda x: generate_section(x, md_doc), sections_list))

    return "%s\n%s" % (head, sections_result)


def normalize(options):
    if options.action == "update" and options.sections is not None:
        raise UtilNormalizeException(correct_pfname(__file__), ["action", "sections"],
                                     "Options <-a update> and <--sections> are incompatable")


def main(options):
    content = generate_doc()

    if options.action == "show":
        print content
    elif options.action == "update":
        s = xmlrpclib.ServerProxy('https://wiki.yandex-team.ru/_api/xmlrpc/')
        auth = s.rpc.login(options.user, getpass())
        s.pages.save(auth, GENCFG_GROUP_CARD_WIKI_URL, content, True, "Описание")
        print "Updated group card documentation on https://wiki.yandex-team.ru%s" % GENCFG_GROUP_CARD_WIKI_URL
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
