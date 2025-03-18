import json
from copy import deepcopy
from collections import defaultdict
from datetime import datetime, timedelta
from antiadblock.tasks.tools.configs_api import get_configs_from_api, post_tvm_request


OBSOLETE_HOURS = 24


class Case:
    def __init__(self, json_data):
        for key, value in json_data.items():
            setattr(self, key, value)
        self.elastic_count = json.loads(self.elastic_count)
        if self.reference_elastic_count is None:
            self.reference_elastic_count = {"url": "", "meta": {"items": 0}, "status": "success"}
        else:
            self.reference_elastic_count = json.loads(self.reference_elastic_count)
        self.start_time = datetime.strptime(json_data["start_time"], '%Y-%m-%dT%H:%M:%S') + timedelta(hours=3)

    def to_dict_of_strings(self):
        attrs = deepcopy(self.__dict__)
        attrs["start_time"] = datetime.strftime(attrs["start_time"], '%Y-%m-%dT%H:%M:%S')
        return attrs


def get_verdict_cases_from_configs_api(configs_api_host, tvm_client):
    api_url = "https://{}/sbs_check/results/verdict_cases".format(configs_api_host)
    return get_configs_from_api(api_url, tvm_client)


def get_adblock_and_reference_cases(results):
    adblock_cases = []
    for item in results["data"]:
        adblock_cases.append(Case(item))
    return adblock_cases


def save_verdict_results_to_configs_api(json_data, configs_api_host, tvm_client):
    api_url = "https://{}/sbs_check/results/logs".format(configs_api_host)
    post_tvm_request(api_url, tvm_client, json_data)


def get_verdict_for_adblock_case(case):
    result = dict(
        sbs_results_id=case.id,
        request_id=case.request_id,
        has_problem='ok',
    )

    if case.reference_elastic_count["meta"].get('items', 0) == 0:
        result['has_problem'] = 'unknown'
        return result

    if case.reference_elastic_count["meta"].get('items', 0) != case.elastic_count["meta"].get('items', 0):
        result['has_problem'] = 'problem'
        return result

    return result


def verdict_task(configs_api_host, tvm_client, logger):
    logger.info("Verdict Task")

    adblock_cases = get_adblock_and_reference_cases(get_verdict_cases_from_configs_api(configs_api_host, tvm_client))

    logger.info("Adblock Cases: {}".format(json.dumps([x.to_dict_of_strings() for x in adblock_cases], indent=2)))

    obsolete_cases = [x for x in adblock_cases if (datetime.now() + timedelta(hours=3)) - x.start_time > timedelta(hours=OBSOLETE_HOURS)]
    adblock_cases = [x for x in adblock_cases if x not in obsolete_cases]

    logger.info("Obsolete Cases: {}".format(json.dumps([x.to_dict_of_strings() for x in obsolete_cases], indent=2)))

    results = []
    results.extend(list(map(get_verdict_for_adblock_case, adblock_cases)))

    logger.info("Results Without Obsolete: {}".format(json.dumps(results, indent=2)))
    results.extend(dict(sbs_results_id=item.id,
                        request_id=item.request_id,
                        has_problem='obsolete') for item in obsolete_cases)

    json_result = defaultdict(list)
    for item in results:
        json_result[item["sbs_results_id"]].append(dict(
            request_id=item["request_id"],
            has_problem=item["has_problem"]
        ))

    save_verdict_results_to_configs_api(json_result, configs_api_host, tvm_client)
