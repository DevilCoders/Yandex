# coding=utf-8


class Cluster(object):
    def __init__(self, name, description):
        self.name = name
        self.description = description


# from https://st.yandex-team.ru/OFFLINE-100#1500940884000
CLUSTERS = {
    0: Cluster(u"games-eng", u"игровые запросы (преимущественно английские)"),
    1: Cluster(u"no-text", u"не распознанный мусор, смайлики, нетекстовые запросы, иероглифы"),
    2: Cluster(u"women", u"Красота- косметика, бады, массажеры и тп, что-то купить"),
    3: Cluster(u"images", u"Поздравления, рисования, сделай сам, фото, картинки?"),
    4: Cluster(u"literature", u"литература (в том числе школьная), стихи, тосты, молитвы тексты, вопросы"),
    5: Cluster(u"currency", u"валютные запросы"),
    6: Cluster(u"programmer", u"программистские запросы (linux, html, 1c, debian, sql)"),
    7: Cluster(u"humanities", u"Гуманитарные науки: литература, история, вопросы (школьные)"),
    8: Cluster(u"devices", u"девайсы, смартфоны, модемы"),
    9: Cluster(u"men", u"авто мото, ремонт, оружие иснотрументы - купить - коммерчесике - мужские - запросы ??"),
    10: Cluster(u"eng-translate", u"похоже на английские запросы на перевод и песни"),
    11: Cluster(u"school", u"школьные запросы"),
    12: Cluster(u"commercial", u"коммерческие?"),
    13: Cluster(u"celebrities", u"люди, сплетни, герои телевизора и тп"),
    14: Cluster(u"regional", u"запросы с указанием города?"),
    15: Cluster(u"foreign", u"иностранные запросы (в том числе наука)"),
    16: Cluster(u"construction", u"строительные?"),
    17: Cluster(u"numbers", u"какие-то странные числовые запросы"),
    18: Cluster(u"unknown", u"хз, шум"),
    19: Cluster(u"law", u"правовые?"),
    20: Cluster(u"games-rus", u"игровые запросы (преимущественно русские)"),
    21: Cluster(u"animals", u"КОТИКИ, запросы про животных?"),
    22: Cluster(u"cars", u"про автомобили"),
    23: Cluster(u"nav", u"навигационные, урловые"),
    24: Cluster(u"regional", u"опять региональные"),
    25: Cluster(u"location", u"места, адреса, города?"),
    26: Cluster(u"tickets", u"афиша, спорт, билеты, купоны, школы, фитнес"),
    27: Cluster(u"prof-soft-hard", u"профессиональные софт - хард запросы(процессоры, устройства, фотошоп, кодеки)"),
    28: Cluster(u"computers", u"про комплектующие, ноутбуки, компьютеры"),
    29: Cluster(u"wiki-person", u"Академическаие личности, википедийные личсности, объект ответ"),
    30: Cluster(u"soft", u"русский софт, начальный софт (кряки, скачать, коды, виндовс)"),
    31: Cluster(u"schemas",
                u"какие-то узкоспециализированные технические запросы, станки, схемы, госты, химия, монтаж"),
    32: Cluster(u"porno-eng", u"английские порнозапросы"),
    33: Cluster(u"food", u"питание, готовка"),
    34: Cluster(u"medicine", u"медицина, вопросы"),
    35: Cluster(u"songs", u"песни?"),
    36: Cluster(u"students", u"рефераты, задания, уровень студентов, отчетов предприятий"),
    37: Cluster(u"movies", u"фильмы, сериалы"),
    38: Cluster(u"porno-rus", u"русские порнозапросы"),
    39: Cluster(u"market", u"техника, купить, фото, видео, бытовая,для авто (маркетподобное)"),
}
