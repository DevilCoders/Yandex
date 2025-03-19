import argparse
import json
import logging
import urllib2

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(formatter)
logger.addHandler(ch)
logger.setLevel(logging.INFO)


HOST = "kikimr0430.search.yandex.net"
KIKIMR_PORT = 8765
SIZE_THRESHOLD = 70
NBS_TABLET_TYPES = {26, 30}


def fetch(url):
    try:
        return urllib2.urlopen(url).read()
    except urllib2.URLError as e:
        logger.exception(
            "Got URLError exception in urlopen: {}".format(e.reason))
    except urllib2.HTTPError as e:
        logger.exception(
            "Got HTTPError exception in urlopen: {}".format(e.code))
    except Exception as e:
        logger.exception("Got exception in urlopen: {}".format(str(e)))
    return ""


def find_overloaded_pdisks(host, port, threshold):
    result = {}
    pdisks = json.loads(fetch("http://{}:{}/viewer/json/pdiskinfo".format(
        host, port)))
    logger.debug("PDisks response: {}".format(pdisks))
    for pdisk in pdisks["PDiskStateInfo"]:
        if "TotalSize" in pdisk and "AvailableSize" in pdisk:
            total_size = int(pdisk["TotalSize"])
            avail_size = int(pdisk["AvailableSize"])
            if 100 - (avail_size * 100 / total_size) >= threshold:
                result[(pdisk["NodeId"], pdisk["PDiskId"])] = pdisk
    return result


def find_groups_on_pdisks(host, port, pdisks):
    result = set()
    groups = json.loads(fetch("http://{}:{}/viewer/json/storage".format(
        host, port)))
    logger.debug("Groups response: {}".format(groups))
    if "BSGroupStateInfo" in groups:
        for group in groups["BSGroupStateInfo"]:
            for vdisk in group["VDisks"]:
                if ("PDisk" in vdisk) and (vdisk["PDisk"]["NodeId"], vdisk["PDisk"]["PDiskId"]) in pdisks:
                    result.add(group["GroupID"])
    return result


def find_nbs_tablets(host, port, groups):
    result = set()
    tablets = json.loads(fetch("http://{}:{}/viewer/json/tabletinfo".format(
        host, port)))
    logger.debug("Tablets response: {}".format(tablets))
    if "TabletStateInfo" in tablets:
        for tablet in tablets["TabletStateInfo"]:
            if ("Type" in tablet) and (tablet["Type"] in NBS_TABLET_TYPES):
                channels = tablet["ChannelGroupIDs"]
                for channel in channels:
                    if channel in groups:
                        result.add(tablet["TabletId"])
    return result


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--host", help="host name", default=HOST)
    parser.add_argument(
        "--kikimr-port", help="kikimr monitoring port", type=int, default=KIKIMR_PORT)
    parser.add_argument(
        "--size-threshold", help="used space/total size ratio", type=int, default=SIZE_THRESHOLD)
    parser.add_argument(
        "--verbose", help="allow debug output", default=False, action="store_true")

    args = parser.parse_args()

    if args.verbose is True:
        logger.setLevel(logging.DEBUG)

    pdisks_to_clean = find_overloaded_pdisks(args.host, args.kikimr_port, args.size_threshold)
    groups_to_clean = find_groups_on_pdisks(args.host, args.kikimr_port, pdisks_to_clean)
    tablets_to_clean = find_nbs_tablets(args.host, args.kikimr_port, groups_to_clean)
    for tablet in tablets_to_clean:
        print(tablet)

run()
