#!/usr/bin/env python
# coding=utf-8

import json
import os
import pwd
import sys

from collections import defaultdict, OrderedDict

import abc_api
import resources_counter
import tools

BASE_DIR = "billing"


class BillingCombinator:
    def __init__(self, abc_map_path=None, prices=None, months=None, debug=False):
        assert months
        self.GOOD_MONTHS = months
        self.RESOURCES_NAMES = ["cpu", "mem", "ram", "hdd", "ssd"]

        if abc_map_path:
            self._load_abc_map(abc_map_path)

        self._prices = prices

        self._abc_api_holder = abc_api.load_abc_api(reload=not debug)

    def _load_abc_map(self, path):
        self.abc_to_top = {}

        for line in file(path):
            data = json.loads(line)
            self.abc_to_top[data["slug"].lower()] = data["top_to_oebs"]

    def correct_resource(self, cloud, resources):
        # using algorithm from https://st.yandex-team.ru/PLN-700#605b2fdce291e37ed0e52d10

        res = {}

        for name, val in resources.iteritems():
            assert name in self.RESOURCES_NAMES

            if name == "mem":
                name = "ram"

            if cloud == "yt":
                if name == "ram":
                    continue

                if name == "cpu":
                    res["ram"] = val * 512 / 80
            elif cloud == "yp":
                pass
            elif cloud in ["mds", "s3"]:
                assert name == "hdd"
            elif cloud in ["gencfg", "qloud"]:
                if name in ["hdd", "ssd"]:
                    continue

                if name == "cpu":
                    res["hdd"] = val * (14 * 1024 / 80)
                    res["ssd"] = val * (3.2 * 1024 / 80)
            elif cloud in ["mdb"]:
                pass
            else:
                raise Exception("unknown cloud: %s" % cloud)

            res[name] = val

        return res

    def evaluate_billing(self, input_data_paths, output_path, correct_resources=True):
        res = []

        for path in input_data_paths:
            file_data = json.load(file(path))

            for cloud, cloud_data in file_data.iteritems():
                for record in cloud_data:
                    month = record["month"].replace(".", "-")
                    if month.lower() == "2021-q1":
                        months = self.GOOD_MONTHS
                    else:
                        assert month in self.GOOD_MONTHS
                        months = [month]
                    for month in months:
                        abc_slug = record.get("abc_slug")

                        if abc_slug is None:
                            abc_id = int(record["abc_id"])
                            abc_slug = self._abc_api_holder.get_service_slug(abc_id)
                        elif abc_slug.isdigit():
                            abc_slug = int(abc_slug)
                            abc_slug = self._abc_api_holder.get_service_slug(abc_slug)

                        if not abc_slug:
                            sys.stderr.write("empty abc_slug at %s: %s\n" % (cloud, record))
                            # TODO do something
                            continue

                        top_abc = record.get("top_abc")
                        if top_abc is None:
                            top_abc = self.abc_to_top.get(abc_slug.lower())
                        if top_abc is None:
                            sys.stderr.write("Can't find top_abc for %s %s: %s\n" % (cloud, abc_slug, record))
                            continue

                        resources = record["resources"]
                        if correct_resources:
                            resources = self.correct_resource(cloud, resources)

                        for name, val in resources.iteritems():
                            temp = OrderedDict()

                            temp["month"] = month
                            temp["abc"] = abc_slug
                            temp["top_abc"] = top_abc

                            temp["cloud"] = cloud
                            temp["sku_name"] = name
                            temp["sku_val"] = val

                            if self._prices is not None:
                                price = self._prices.get(name)

                                if price is None:
                                    price = self._prices.get("%s.%s" % (cloud, name))

                                if price is None:
                                    print("No price for %s %s" % (cloud, name))
                                    continue

                                cost = val * price

                                if correct_resources and cloud in ["mds", "s3", "mdb"] and name in ["hdd", "ssd"]:
                                    cost *= 2.5

                                temp["cost"] = cost

                            res.append(temp)

        resources_counter.save_csv(res, output_path, delimeter=',')


def convert_rtc_data(inpath, out_path, mode):
    assert mode

    RTC_CLOUDS = "gencfg qloud yp".split()

    sku_map = {
        "cpu": "cpu",
        "memory": "mem",
        "hdd_storage": "hdd",
        "ssd_storage": "ssd",
    }

    res = defaultdict(list)

    for line in file(inpath):
        data = json.loads(line)

        month = data["month"]
        assert month.count("-") == 2
        month = "-".join(month.split("-")[:2])

        abc = data["abc_slug"]
        cloud = data["service_name"]
        if cloud not in RTC_CLOUDS:
            continue

        sku = data["sku_name"]
        sku_val = data["billing_record_pricing_quantity"]

        sku_cloud, sku_name, sku_type = sku.split(".")
        assert sku_cloud == cloud

        if sku_type != "quota":
            continue

        real_sku_name = sku_map.get(sku_name)

        if not real_sku_name:
            continue

        if mode == "zen":  # https://st.yandex-team.ru/MCF-182#60799dcaef171b207e90657f
            assert month == "2021-03"
            month = "2021-Q1"
            sku_val = float(sku_val) * 31 / 11
        elif mode == "april":
            month = "2021-04"
            sku_val = float(sku_val) * 30 / 25
        elif mode == "may":
            month = "2021-05"
            sku_val = float(sku_val) * 30 / 27  # TODO do it right
        elif mode == "2021-q1":
            if month == "2021-03":
                # partial data correction https://st.yandex-team.ru/PLN-700#605b58b1f5d9884a77ad1a16
                sku_val = float(sku_val) * 31 / 23
        else:
            assert False

        res[cloud].append({
            "month": month,
            "abc_slug": abc,
            "resources": {
                real_sku_name: sku_val
            }
        })

    tools.dump_json(res, out_path)


def convert_billing_data(inpath, out_path, new_sku=False, prices=None):
    chosen_clouds = "gencfg qloud yp yt".split()
    clouds_renames = {}
    all_clouds = set()
    volumes_mult = {}
    clouds_by_prefix = []

    if new_sku:
        chosen_clouds = "gencfg qloud yp yt storage mds avatars mdb.ch mdb.elasticsearch mdb.kafka mdb.mongo mdb.mysql mdb.pg mdb.redis saas".split()
        clouds_renames = {
            "storage": "s3"
        }

        clouds_by_prefix = [
            "yt"
        ]

        sku_prefixes_map = [
            ("gencfg.", "rtc."),
            ("qloud.", "rtc."),
            ("yp.", "rtc."),

            ("yt.hahn.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.arnold.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.hume.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.freud.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.pythia.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.vanga.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.bohr.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),
            ("yt.landau.dynamic_tables.tablet_cell", "yt.big_clusters.tablet_cell"),

            ("yt.seneca-sas.dynamic_tables.tablet_cell", "yt.dyntables_clusters.tablet_cell"),
            ("yt.seneca-man.dynamic_tables.tablet_cell", "yt.dyntables_clusters.tablet_cell"),
            ("yt.seneca-vla.dynamic_tables.tablet_cell", "yt.dyntables_clusters.tablet_cell"),
            ("yt.markov.dynamic_tables.tablet_cell", "yt.dyntables_clusters.tablet_cell"),
            ("yt.zeno.dynamic_tables.tablet_cell", "yt.dyntables_clusters.tablet_cell"),

            ("yt.hahn.", "yt."),
            ("yt.arnold.", "yt."),
            ("yt.hume.", "yt."),
            ("yt.freud.", "yt."),
            ("yt.seneca-sas.", "yt."),
            ("yt.seneca-man.", "yt."),
            ("yt.seneca-vla.", "yt."),
            ("yt.markov.", "yt."),
            ("yt.zeno.", "yt."),
            ("yt.pythia.", "yt."),
            ("yt.vanga.", "yt."),
            ("yt.bohr.", "yt."),
            ("yt.landau.", "yt."),

            ("s3.", "s3."),
            ("mds.", "mds."),
            ("avatars.", "avatars."),

            ("mdb.", "mdb."),

            ("saas.", "saas.")
        ]

        sku_map = {
            # RTC compute
            'rtc.cpu.quota': 'rtc.cpu',
            'rtc.memory.quota': 'rtc.ram',
            'rtc.hdd_storage.quota': 'rtc.hdd',
            'rtc.ssd_storage.quota': 'rtc.ssd',
            'rtc.cpu.order': 'rtc.cpu',
            'rtc.memory.order': 'rtc.ram',
            'rtc.hdd_storage.order': 'rtc.hdd',
            'rtc.ssd_storage.order': 'rtc.ssd',

            # RTC GPU
            'rtc.gpu_geforce_1080ti.quota': 'rtc.gpu_geforce_1080ti',
            'rtc.gpu_tesla_k40.quota': 'rtc.gpu_tesla_k40',
            'rtc.gpu_tesla_m40.quota': 'rtc.gpu_tesla_m40',
            'rtc.gpu_tesla_v100.quota': 'rtc.gpu_tesla_v100',
            'rtc.gpu_tesla_v100.order': 'rtc.gpu_tesla_v100',

            # YT compute
            'yt.compute.strong_guarantee.cpu': 'yt.cpu_guarantee',
            'yt.compute.integral_guarantee.burst.cpu': 'yt.cpu_burst',
            'yt.compute.integral_guarantee.relaxed.cpu': 'yt.cpu_relaxed',
            'yt.compute.usage.cpu': 'yt.cpu_usage',
            'yt.compute.usage.memory': 'yt.ram_usage',
            'yt.disk_space.hdd': 'yt.hdd',
            'yt.disk_space.ssd': 'yt.ssd',

            # YT GPU
            'yt.gpu.geforce_1080ti.strong_guarantee.gpu': 'yt.gpu_geforce_1080ti',
            'yt.gpu.geforce_1080ti.usage.gpu': 'yt.gpu_geforce_1080ti_usage',
            'yt.gpu.tesla_a100.strong_guarantee.gpu': 'yt.gpu_tesla_a100',
            'yt.gpu.tesla_a100.usage.gpu': 'yt.gpu_tesla_a100_usage',
            'yt.gpu.tesla_k40.strong_guarantee.gpu': 'yt.gpu_tesla_k40',
            'yt.gpu.tesla_k40.usage.gpu': 'yt.gpu_tesla_k40_usage',
            'yt.gpu.tesla_m40.strong_guarantee.gpu': 'yt.gpu_tesla_m40',
            'yt.gpu.tesla_m40.usage.gpu': 'yt.gpu_tesla_m40_usage',
            'yt.gpu.tesla_p40.strong_guarantee.gpu': 'yt.gpu_tesla_p40',
            'yt.gpu.tesla_p40.usage.gpu': 'yt.gpu_tesla_p40_usage',
            'yt.gpu.tesla_v100.strong_guarantee.gpu': 'yt.gpu_tesla_v100',
            'yt.gpu.tesla_v100.usage.gpu': 'yt.gpu_tesla_v100_usage'
        }

        for sku in prices.iterkeys():
            sku_cloud = sku.split(".")[0]
            if sku_cloud == "mdb":
                volumes_mult[sku] = "day_hours"
            elif sku_cloud in ["mds", "s3", "avatars"]:
                sku_split = sku.split(".")
                if "quota_space" in sku_split:
                    volumes_mult[sku] = "day_hours"
                else:
                    mult = None
                    for el in "get/head/options".split("/"):
                        if el in sku_split:
                            assert mult is None
                            mult = 10000
                            break
                    for el in "put/post/delete".split("/"):
                        if el in sku_split:
                            assert mult is None
                            mult = 1000
                            break

                    if mult:
                        volumes_mult[sku] = mult

    else:
        sku_prefixes_map = [
            "gencfg.",
            "qloud.",
            "yp.",
            "yt.hahn.",
            "yt.arnold.",
            "yt.hume.",
            "yt.freud.",
            "yt.seneca-sas.",
            "yt.seneca-man.",
            "yt.seneca-vla.",
            "yt.markov.",
            "yt.zeno.",
            "yt.pythia.",
            "yt.vanga.",
        ]
        sku_prefixes_map = dict([(el, "") for el in sku_prefixes_map])

        sku_map = {
            # RTC compute
            'cpu.quota': 'cpu',
            'memory.quota': 'ram',
            'hdd_storage.quota': 'hdd',
            'ssd_storage.quota': 'ssd',

            # YT
            'compute.strong_guarantee.cpu': 'cpu',
            'disk_space.hdd': 'hdd',
            'disk_space.ssd': 'ssd',
        }

    for sku in prices.iterkeys():
        sku_map[sku] = sku

    skipped = set()

    res = defaultdict(list)

    for line in file(inpath):
        data = json.loads(line)

        month = data["month"]
        assert month.count("-") == 2
        month = "-".join(month.split("-")[:2])

        abc = data["abc_slug"]
        cloud = data["service_name"].lower()
        all_clouds.add(cloud)
        if cloud not in chosen_clouds:
            continue

        cloud = clouds_renames.get(cloud, cloud)

        sku = data["sku_name"]
        sku_name = None
        for prefix, replacement in sku_prefixes_map:
            if sku.startswith(prefix):
                sku_name = sku.replace(prefix, replacement, 1)
                if cloud in clouds_by_prefix:
                    cloud = '.'.join(prefix.split('.')[:2])

                break

        if not sku_name:
            continue

        real_sku_name = sku_map.get(sku_name)

        if not real_sku_name:
            skipped.add(sku)
            continue

        sku_val = data["billing_record_pricing_quantity"]

        if month == "2021-04":  # https://st.yandex-team.ru/PLN-738#6089450748f9d726df75b9d9
            month_days = 30
            sku_val = float(sku_val) * month_days / 27
        elif month == "2021-05":  # https://st.yandex-team.ru/MCF-209#60afa2278ea839550c246824
            month_days = 31
            sku_val = float(sku_val) * month_days / 26
        elif month == "2021-06":  # https://st.yandex-team.ru/PLN-776#60d4509e005df879a9465fa4
            month_days = 30
            sku_val = float(sku_val) * month_days / 27
        elif month == "2021-07":  # https://st.yandex-team.ru/PLN-845#61097bb4d2ddb977c3be042f
            month_days = 31
        else:
            assert False

        mult = volumes_mult.get(real_sku_name)
        if mult:
            if mult == "day_hours":
                sku_val = float(sku_val) / month_days / 24
            elif isinstance(mult, (int, float)):
                sku_val = float(sku_val) * mult
            else:
                assert False

        res[cloud].append({
            "month": month,
            "abc_slug": abc,
            "top_abc": data["top_to_oebs"],
            "resources": {
                real_sku_name: sku_val
            }
        })

    tools.dump_json(res, out_path)

    sys.stderr.write("all clouds:\n%s\n\n" % "\n".join(sorted(all_clouds)))
    sys.stderr.write("skipped sku:\n%s\n\n" % "\n".join(sorted(skipped)))


def convert_mdb_data(inpath, outpath):
    import pandas

    res = []

    bytes_to_gb = float(1024 * 1024 * 1024)

    for row in pandas.read_csv(inpath).itertuples():
        temp = {
            "month": "2021-Q1",
            "abc_slug": row.abc_slug,
            "resources": {
                "cpu": row.cpu_quota,
                "mem": row.memory_quota / bytes_to_gb,
                "hdd": row.hdd_space_quota / bytes_to_gb,
                "ssd": row.ssd_space_quota / bytes_to_gb,
            }
        }

        res.append(temp)

    res = {
        "mdb": res
    }

    tools.dump_json(res, outpath)


def billing(mode=None, debug=False):
    new_sku = False
    correct_resources = True

    billing_data_path = os.path.join(BASE_DIR, "billing_data.json")
    billing_data_converted_path = os.path.join(BASE_DIR, "billing_data_converted.json")

    months = ["2021-01", "2021-02", "2021-03"]

    if mode == "zen":
        billing_data_path = os.path.join(BASE_DIR, "billing_data_zen.json")

    prices = json.load(file(os.path.join(BASE_DIR, "prices.json")))

    if mode == "april":
        months = ["2021-04"]

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "mdb_2021_04.json"),
            os.path.join(BASE_DIR, "yt_resources_2021_april.json"),
            os.path.join(BASE_DIR, "mds.2021.04.json"),
            os.path.join(BASE_DIR, "s3.2021.04.json"),
        ]
    elif mode == "may":
        months = ["2021-05"]

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "mdb_2021_05.json"),
            # os.path.join(BASE_DIR, "yt_resources_2021_may.json"),
            os.path.join(BASE_DIR, "mds.2021.05.json"),
            os.path.join(BASE_DIR, "s3.2021.05.json"),
        ]
    elif mode == "may-new":
        months = ["2021-05"]

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "net_2021-05.json"),
            os.path.join(BASE_DIR, "distbuild.json"),
            os.path.join(BASE_DIR, "sandbox.json"),
        ]

        new_sku = True
        correct_resources = False
    elif mode == "july-new":
        months = ["2021-07"]

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "net_2021-07.json"),
            os.path.join(BASE_DIR, "distbuild.json"),
            os.path.join(BASE_DIR, "sandbox.json"),
            os.path.join(BASE_DIR, "saas.json"),
            os.path.join(BASE_DIR, "hardware.json"),
            os.path.join(BASE_DIR, "strm.json"),
            os.path.join(BASE_DIR, "yt_july_gpu_a100_80g.json"),
            os.path.join(BASE_DIR, "yt_research_usage.json")
        ]

        new_sku = True
        correct_resources = False
    elif mode == "july-new-research":
        months = ["2021-07"]

        input_data_paths = [
            os.path.join(BASE_DIR, "yt_research_usage.json")
        ]

        new_sku = True
        correct_resources = False
    elif mode == "june":
        months = ["2021-06"]

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "mdb_2021_06.json"),
            os.path.join(BASE_DIR, "mds.2021.06.json"),
            os.path.join(BASE_DIR, "s3.2021.06.json"),
        ]
    else:
        mdb_data_path = os.path.join(BASE_DIR, "mdb_abc_quota_2021_03.csv")
        mdb_converted_path = os.path.join(BASE_DIR, "mdb_data.json")
        convert_mdb_data(mdb_data_path, mdb_converted_path)

        input_data_paths = [
            billing_data_converted_path,
            os.path.join(BASE_DIR, "s3_stat_quota.json"),
            os.path.join(BASE_DIR, "mds_stat_quota.json"),
            os.path.join(BASE_DIR, "yt_resources_2021_q1.json"),
            mdb_converted_path,
        ]

    convert_billing_data(billing_data_path, billing_data_converted_path, new_sku=new_sku, prices=prices)

    output_path = os.path.join(BASE_DIR, "billing_result.csv")

    combinator = BillingCombinator(
        abc_map_path=os.path.join(BASE_DIR, "abc_to_top.json"),
        prices=prices,
        months=months,
        debug=debug
    )

    combinator.evaluate_billing(input_data_paths, output_path, correct_resources=correct_resources)

    print output_path


def pln_738(debug=False):
    billing_data_path = os.path.join(BASE_DIR, "billing_data.json")
    converted_data_path = os.path.join(BASE_DIR, "billing_data_converted.json")

    convert_billing_data(billing_data_path, converted_data_path, new_sku=True)

    combinator = BillingCombinator(
        months=["2021-01", "2021-02", "2021-03", "2021-04"],
        debug=debug
    )

    input_data_paths = [converted_data_path]

    output_path = os.path.join(BASE_DIR, "billing_result.csv")

    combinator.evaluate_billing(input_data_paths, output_path, correct_resources=False)

    print output_path


def bu_resp_list(bu_only=False, debug=False):
    if debug:
        yield "test", ["sereglond"]
        return

    resp_list_data = u"""
    Вертикали	svyatoslav@ ibiryulin@ kchermenskaya@ dashikin@ fresk@yandex-team.ru r-abashidze@
    Cloud		danilapavlov@yandex-team.ru  dashikin@
    Дзен	dkondra@yandex-team.ru	vdedova@yandex-team.ru  dashikin@
    Едадил	mosquito@yandex-team.ru	e-shavrina@yandex-team.ru  dashikin@ t-romanova@ rassuditelnov@
    Финтех	spleenjack@yandex-team.ru	dashikin@yandex-team.ru, vdedova@yandex-team.ru
    Гео khrolenko@yandex-team.ru "sorokovaya@yandex-team.ru lunaduxa@yandex-team.ru ta-kra@yandex-team.ru"
    Медиа	ignatich@yandex-team.ru	"evbrodskaya@yandex-team.ru ivandt@yandex-team.ru"
    Маркет	le087@yandex-team.ru	shmakova-elen@yandex-team.ru igor-askarov@ dvkhromov@
    Образование	w495@yandex-team.ru	e-shavrina@yandex-team.ru
    Такси	khomikki@yandex-team.ru	alina-greb@yandex-team.ru elya84@yandex-team.ru	eatroshkin@yandex-team.ru
    SDC	riddle@yandex-team.ru	pristavka@yandex-team.ru	ilzar@yandex-team.ru
    Услуги	mborisov@yandex-team.ru	vdedova@yandex-team.ru dashikin@
    Драйв	rureggaeton@yandex-team.ru	igorzaysev@yandex-team.ru
    """

    if not bu_only:
        resp_list_data += u"""
        Поисковый портал (VS) mvel@yandex-team.ru pavelgorch@yandex-team.ru
        Персональные сервисы (VS)  ignition@yandex-team.ru zanzara@yandex-team.ru
        Суперапп (VS)	golubtsov@yandex-team.ru	timy4@yandex-team.ru	olga-levkina@yandex-team.ru pavelgorch@yandex-team.ru
        FinOps Platform (VS)    devamax@yandex-team.ru	eyuzh@yandex-team.ru ilonager@yandex-team.ru, tabolin@yandex-team.ru, dskut@yandex-team.ru maximkovalev@yandex-team.ru	e-filkina@yandex-team.ru
        Алиса (VS)	avitella@yandex-team.ru, rnefyodov@yandex-team.ru 		aiskandarov@yandex-team.ru chizhik@yandex-team.ru
        Умные устройства avitella@yandex-team.ru aiskandarov@
        Путешествия (VS)	lorekhov@yandex-team.ru	reyder@yandex-team.ru	katyadeeva@yandex-team.ru
        Безопасность (VS)	sterh@yandex-team.ru	tokza@yandex-team.ru	katyadeeva@yandex-team.ru
        Информационные сервисы (VS)	sgrb@yandex-team.ru	crazysolntse@yandex-team.ru	katyadeeva@yandex-team.ru
        Яндекс.Кью (VS)	nexidan@yandex-team.ru	tosamsonova@yandex-team.ru	olga-levkina@yandex-team.ru
        PR (VS)	kanatnikov@yandex-team.ru	aramhardy@yandex-team.ru	chizhik@yandex-team.ru
        Маркетинг (VS)	abroskin@yandex-team.ru	chizhik@yandex-team.ru
        SMB (VS)	lupach@yandex-team.ru	"vladbelugin@yandex-team.ru asikorsky@yandex-team.ru baksheev@yandex-team.ru"	irinakozinets@yandex-team.ru e-rasteryaeva@yandex-team.ru
        # Реклама (VS) bahbka@yandex-team.ru katyadeeva@yandex-team.ru sankear@, zipzipzip@yandex-team.ru, epic@yandex-team.ru; idzhobava@yandex-team.ru, artalex@yandex-team.ru
        Краудсорсинг (VS) leftie@yandex-team.ru,   olga-levkina@yandex-team.ru  nosetrov@  adrutsa@yandex-team.ru
        G&A (VS) keyd@yandex-team.ru chizhik@yandex-team.ru
        Толока (VS) ortemij@yandex-team.ru
        Экосистемные продукты ID+Pay (VS)  devamax@yandex-team.ru	lyadzhin@yandex-team.ru,	e-filkina@yandex-team.ru
        Маршрутизация   4c4d@yandex-team.ru lunaduxa@yandex-team.ru
        ОПК (VS) dmitryno@yandex-team.ru lunaduxa@yandex-team.ru
        """

    for line in resp_list_data.split("\n"):
        line = line.replace("\"", " ").replace(",", " ").replace(";", " ").split()
        if not line or line[0].startswith("#"):
            continue

        bu = []
        resp = []

        for el in line:
            if "@" in el:
                resp.append(el.split("@")[0])
            else:
                assert not resp
                bu.append(el)

        bu = " ".join(bu)

        yield bu, resp


def print_resp_list():
    res = set()
    for bu, resp in sorted(bu_resp_list()):
        print bu, ", ".join(resp)
        res.update(resp)

    print
    print ", ".join([el + "@yandex-team.ru" for el in sorted(res)])


def _find_child(children, summary):
    for task in children:
        if task.summary == summary:
            return task


def monthly_tickets(debug=True):
    import startrek

    queue = "PLN"
    components = ["closed"]
    followers_extra = ["inna-k"]

    month = u"июль"
    year = 2022
    head_summary = u"Проверка счетов за {month} {year}".format(month=month, year=year)
    meta_head = startrek.get_issue("PLN-901")
    children = [startrek.get_issue(link.object.key) for link in meta_head.links if
                link.type.id in ["depends", "subtask"] and link.direction == "outward"]

    head = _find_child(children, head_summary)

    if head is None:
        head = startrek.create_issue(
            queue=queue,
            summary=head_summary,
            components=components
        )

        startrek.link_issues(meta_head, head, 'is parent task for')

    print "head task:", head.key

    copy_task = startrek.get_issue("PLN-903")

    children = [startrek.get_issue(link.object.key) for link in head.links if
                link.type.id in ["depends", "subtask"] and link.direction == "outward"]

    old_access = sorted([el.id for el in head.access])
    all_resp = set(old_access)

    for bu, resp in bu_resp_list(debug=debug):
        task_summary = copy_task.summary
        task_summary = task_summary.format(month=month, year=year, bu=bu)

        description = copy_task.description.format(month=month, year=year, bu=bu)

        task = _find_child(children, task_summary)

        if task is None:
            task = startrek.create_issue(
                queue=queue,
                summary=task_summary,
                description=description,
                components=components,
                assignee=resp[0],
                followers=resp + followers_extra,
            )

            task.comments.create(text=task_summary, summonees=resp)

            startrek.link_issues(head, task, 'is parent task for')

        if task.description != description:
            task.update(description=description)

        all_resp.update([el.id for el in task.access + task.followers])

        print bu, resp, task.key if task else None

    new_access = sorted(all_resp)
    if old_access != new_access:
        head = startrek.get_issue(head.key)
        head.update(access=new_access)


def rates_tickets(debug=False, comment=False):
    import startrek

    queue = "PLN"
    components = ["closed"]
    followers_extra = ["chubinskiy", "inna-k"]

    head = startrek.get_issue("PLN-845")
    copy_task = startrek.get_issue("PLN-847")

    children = [startrek.get_issue(link.object.key) for link in head.links if
                link.type.id in ["depends", "subtask"] and link.direction == "outward"]

    if comment:
        comment = sys.stdin.read().decode("utf-8")

    old_access = sorted([el.id for el in head.access])
    all_resp = set(old_access)

    whoami = pwd.getpwuid(os.getuid()).pw_name

    for bu, resp in bu_resp_list(debug=debug):
        all_resp.update(resp)

        task_summary = copy_task.summary
        task_summary = task_summary.format(bu=bu)

        description = copy_task.description

        task = _find_child(children, task_summary)

        if task is None:
            task = startrek.create_issue(
                queue=queue,
                summary=task_summary,
                description=description,
                components=components,
                assignee=resp[0],
                followers=resp + followers_extra,
            )
            startrek.link_issues(head, task, 'is parent task for')

        print bu, resp, task.key if task else None

        if task is None:
            continue

        all_resp.update([el.id for el in task.access + task.followers])

        if task.description != description:
            task.update(description=description)

        if comment:
            cc = task.followers
            cc.append(task.createdBy)
            cc = [el for el in cc if el.login != whoami]
            cc = sorted(set(cc))

            task.comments.create(text=comment, summonees=cc)

    new_access = sorted(all_resp)
    if old_access != new_access:
        head.update(access=new_access)


def test():
    return


def main():
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--billing", action="store_true")
    parser.add_option("--pln-738", action="store_true")
    parser.add_option("--mode")
    parser.add_option("--resp-list", action="store_true")
    parser.add_option("--rates-tickets", action="store_true")
    parser.add_option("--monthly-tickets", action="store_true")
    parser.add_option("--comment", action="store_true")
    parser.add_option("--do", action="store_true")
    parser.add_option("--debug", action="store_true")
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.billing:
        billing(debug=options.debug, mode=options.mode)
    if options.pln_738:
        pln_738(debug=options.debug)
    if options.resp_list:
        print_resp_list()
    if options.monthly_tickets:
        monthly_tickets(debug=not options.do)
    if options.rates_tickets:
        rates_tickets(debug=not options.do, comment=options.comment)
    if options.test:
        test()


if __name__ == "__main__":
    main()
