import logging

import mstand_utils.testid_helpers as utestid
import yaqutils.misc_helpers as umisc
import yaqutils.requests_helpers as urequests
import yaqutils.time_helpers as utime  # noqa
from yaqab.ab_client import AbClient  # noqa


def find_missed_testids(expected, actual):
    """
    :type expected: list[str]
    :type actual: list[str]
    """
    expected = set(expected)
    actual = set(actual)
    unexpected = actual - expected
    if unexpected:
        raise Exception("Unexpected testids: {}".format(", ".join(sorted(unexpected))))
    missed = expected - actual
    if missed:
        logging.warning("There is no data for some testids: %s", ", ".join(sorted(missed)))

    return missed


def _get_testids_info(client, testids):
    """
    :type client: AbClient
    :type testids: list[str]
    :type full: bool
    :rtype: list[dict[str]]
    """
    if not all(t is not None for t in testids):
        # MSTAND-726
        raise Exception("Some of requested testids are empty. Looks like API misuse.")

    logging.debug("_get_testid_info testids: %s", testids)
    return client.get_testids_info(testids)


def get_testids_info_batched(client, testids):
    """
    :type client: AbClient
    :type testids: list[str]
    :rtype: iter[dict[str]]
    """
    # bucket size was decreased from 200 to 80 in order to decrease
    # the length of URL in GET query to ab - MSTAND-1537
    # and then to 40 due to long testids - MSTAND-1629
    bucket = 40
    for batch in umisc.split_by_chunks_iter(testids, bucket):
        for info in _get_testids_info(client, batch):
            yield info


def get_testid_info_dict(client, testids):
    """
    :type client: AbClient
    :type testids: list[str]
    :rtype: dict[str, dict[str]]
    """

    logging.info("get_testid_info")
    result = {str(x["testid"]): x for x in get_testids_info_batched(client, testids)}
    logging.info("find_missed_testids")
    result.update((testid, {}) for testid in find_missed_testids(testids, result.keys()))
    return result


def get_query_basket_tags(basket_id, basked_tags_cache):
    """
    :type basket_id: str
    :type basked_tags_cache: dict[str, list[str]]
    :rtype: list[str]
    """
    if basket_id in basked_tags_cache:
        return basked_tags_cache[basket_id]

    logging.info("Query basket %s not in cache, peforming request", basket_id)

    metrics_url = "http://metrics.yandex-team.ru/services/api/queriesgroup"
    params = {"id": basket_id}
    queries_group_info = urequests.retry_request("get", metrics_url, params=params).json()
    if not queries_group_info:
        logging.warning("Metrics returned empty result for queries group id {}".format(basket_id))
        return []

    result_first_item = queries_group_info[0]
    if "tags" not in result_first_item:
        logging.warning("No 'tags' field for queries group id {}".format(basket_id))
        return []

    tags = result_first_item["tags"]

    if not isinstance(tags, list):
        logging.warning("'tags' field is not a list for queries group id {}".format(basket_id))

    basked_tags_cache[basket_id] = tags

    return tags


def get_testid_serpset(client, testid, query_basket_tags_cache):
    """
    :type client: AbClient
    :type testid: str
    :type query_basket_tags_cache: dict
    :rtype: str
    """
    if not utestid.testid_is_simple(testid):
        return None

    serpset_infos = client.get_serpset(testid)
    if not serpset_infos:
        return None

    for index, serpset_info in enumerate(serpset_infos):
        basket_id = serpset_info.get("queries_group_id")
        if basket_id is None:
            logging.warning("No 'queries_group_id' in serpset info #%d for testid, skipping.", index, testid)
            continue

        basket_id = str(basket_id)
        logging.info("Testid %s, serpset #%d: basket id = %s", testid, index, basket_id)
        basket_tags = get_query_basket_tags(basket_id, query_basket_tags_cache)
        if "VALIDATE" in basket_tags:
            return _extract_serpset_id(testid, serpset_info)
        else:
            logging.info("Testid %s, serpset #%d: basket is not 'validate', skipping. Basket tags: %s",
                         testid, index, basket_tags)

    logging.info("No 'validate' serpset found for testid %s", testid)


def _extract_serpset_id(testid, serpset_desc):
    """
    :type testid: str
    :param serpset_desc: dict
    :return: str
    """
    serpset_status = serpset_desc["status"]
    if serpset_status == "FAILED":
        logging.info("Serpset for testid %s is FAILED, skipping it", testid)
        return None

    serpset_id = serpset_desc.get("serp_set_id")
    if not serpset_id:
        logging.warning("Serpset status is not 'FAILED', but serp_set_id is not specified for testid %s", testid)

    logging.info("Testid %s: serpset id = %s", testid, serpset_id)
    return str(serpset_id)


def get_split_change_info(client, testids, dates):
    """
    :type client: AbClient
    :type testids: list[str] | tuple
    :type dates: utime.DateRange
    :return: dict[str]
    """
    bad_testids = [testid for testid in testids if not utestid.testid_is_simple(testid)]
    if bad_testids:
        logging.warning("cannot get split change info for some testids: %s", bad_testids)
        testids = [testid for testid in testids if utestid.testid_is_simple(testid)]

    if not testids:
        return {}

    return client.get_testid_split_change(testids, dates)
