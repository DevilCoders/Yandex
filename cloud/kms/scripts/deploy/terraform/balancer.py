import logging
import subprocess
import requests
import time
import yaml


from . import dnsresolve
from . import fail
from . import tokens

BACKOFF = 5.0

# Wait 10 minutes for the RS state to update.
WAIT_FOR_L3TT = 600

# Wait 10 minutes for deploying the new L3tt configuration.
WAIT_FOR_L3TT_DEPLOY = 600

L3TT_NUM_RETRIES = 5

# Wait one minute for the YLB states to update
WAIT_FOR_YLB = 60


def hasYcp():
    try:
        subprocess.check_output(["ycp", "version"])
        return True
    except Exception as e:
        logging.debug("Caught exception when running 'ycp version': %s", e)
        return False


def checkBalancers(balancersSpec, fqdns, ycpProfile, ylbIds, downtimeFqdns):
    for balancer in balancersSpec:
        if balancer["type"].lower() == "l3tt":
            checkBalancerL3ttImpl(balancer, fqdns, downtimeFqdns)
        elif balancer["type"].lower() == "ylb":
            if "ylb-id" in balancer:
                checkBalancerYlb(ycpProfile, balancer["ylb-id"], downtimeFqdns)
            else:
                for ylbId in ylbIds:
                    checkBalancerYlb(ycpProfile, ylbId, downtimeFqdns)
        else:
            logging.error("Unknown balancer type %s", balancer["type"])


def checkBalancerL3ttImpl(balancer, fqdns, downtimeFqdns):
    svcId = str(balancer["service-id"])
    logging.info("Waiting for l3tt reals for %s to become active...", svcId)

    targetIps = dict((fqdn, dnsresolve.resolveFqdn(fqdn)) for fqdn in fqdns)

    deadlineTime = time.time() + WAIT_FOR_L3TT
    inactive = set()
    while time.time() < deadlineTime:
        vsIds = getL3ttVsIds(svcId)
        if len(vsIds) == 0:
            logging.debug("Still no VS ids, retrying")
            time.sleep(BACKOFF)
            continue

        ips = {}
        for vsId in vsIds:
            for fqdn, ip, state in getL3ttRsState(vsId):
                ips[fqdn] = ip
                if state != "ACTIVE":
                    inactive.add(fqdn)
                else:
                    inactive.discard(fqdn)

        if inactive.issubset(downtimeFqdns) and subMap(targetIps, downtimeFqdns) == subMap(ips, downtimeFqdns):
            if len(inactive) > 0:
                logging.info("All reals are active (except downtimed %s)", ", ".join(inactive))
            else:
                logging.info("All reals are active")
            return
        logging.debug("Still not active: %s", inactive)

        if ips != targetIps:
            logging.info("Target IPs:\n%s", "\n".join(" - " + i + ": " +
                                                      targetIps[i] for i in sorted(targetIps.keys())))
            logging.info("Current IPs:\n%s", "\n".join(" - " + i + ": " + ips[i] for i in sorted(ips.keys())))
            updateL3ttReals(svcId, fqdns)
            # Reset the deadline after configuration update.
            deadlineTime = time.time() + WAIT_FOR_L3TT

        time.sleep(BACKOFF)

    raise fail.FailException("Waiting for l3tt timed out, still inactive {0}".format(", ".join(inactive)))


def subMap(m, without):
    ret = {}
    for k, v in m.items():
        if k not in without:
            ret[k] = v
    return ret


def l3ttApiGet(url):
    finalUrl = "https://l3-api.tt.yandex-team.ru/api/v1/" + url
    for i in range(L3TT_NUM_RETRIES):
        try:
            logging.debug("HTTP GET %s", finalUrl)
            headers = {"Authorization": "OAuth " + tokens.getYavToken()}
            return requests.get(finalUrl, headers=headers).json()
        except requests.exceptions.ConnectionError as e:
            logging.debug("Error connecting to %s: %s", finalUrl, e)
        except requests.exceptions.Timeout as e:
            logging.debug("Timeout connecting to %s: %s", finalUrl, e)

        time.sleep(BACKOFF)

    raise fail.FailException("Could not connect to " + finalUrl)


def l3ttApiPost(url, data):
    finalUrl = "https://l3-api.tt.yandex-team.ru/api/v1/" + url
    for i in range(L3TT_NUM_RETRIES):
        try:
            logging.debug("HTTP POST %s with data %s", finalUrl, data)
            headers = {"Authorization": "OAuth " + tokens.getYavToken()}
            return requests.post(finalUrl, headers=headers, data=data).json()
        except requests.exceptions.ConnectionError as e:
            logging.debug("Error connecting to %s: %s", finalUrl, e)
        except requests.exceptions.Timeout as e:
            logging.debug("Timeout connecting to %s: %s", finalUrl, e)

        time.sleep(BACKOFF)

    raise fail.FailException("Could not connect to " + finalUrl)


def getL3ttVsIds(svcId):
    r = l3ttApiGet("/service/" + svcId)
    ret = []
    for o in r["vs"]:
        if any(s["state"] == "ANNOUNCED" for s in o["status"]):
            ret.append(o["ext_id"])
    return ret


def getL3ttRsState(vsId):
    r = l3ttApiGet("/vs/" + vsId + "/rsstate")
    ret = []
    for o in r["objects"]:
        ret.append((o["rs"]["fqdn"], o["rs"]["ip"], o["state"]))
    return ret


def updateL3ttReals(svcId, fqdns):
    logging.info("Updating l3tt configuration for %s after IP change...", svcId)
    deadlineTime = time.time() + WAIT_FOR_L3TT_DEPLOY

    data = {"groups": "\n".join(fqdns)}
    r = l3ttApiPost("/service/" + svcId + "/editrs", data)
    if r["result"] != "OK":
        raise fail.FailException("Error while updating l3tt reals: " + r["message"])

    configId = str(r["object"]["id"])
    logging.info("Got new configuration %s", configId)

    # There are cases when the tunnel interface does not get created until the L3 configuration gets deployed.
    data = {"force": "true"}
    l3ttApiPost("/service/" + svcId + "/config/" + configId + "/process", data)
    if r["result"] != "OK":
        raise fail.FailException("Error while updating l3tt reals: " + r["message"])

    while time.time() < deadlineTime:
        r = l3ttApiGet("/service/" + svcId + "/config/" + configId)
        state = r["state"]
        if state == "ACTIVE":
            logging.info("Updated l3tt configuration, waiting for state...")
            return
        elif state in ["TEST_FAIL", "VCS_FAIL", "FAIL", "INACTIVE"]:
            raise fail.FailException(
                "Configuration {0} for l3tt service {1} ended up with state {2}".format(configId, svcId, state))

        time.sleep(BACKOFF)


def checkBalancerYlb(ycpProfile, ylbId, downtimeFqdns):
    logging.info("Waiting for ylb %s to become healthy...", ylbId)
    deadlineTime = time.time() + WAIT_FOR_YLB

    targetGroupIds = getYlbTargetGroupIds(ycpProfile, ylbId)
    inactive = set()
    while time.time() < deadlineTime:
        for tgId in targetGroupIds:
            states = getYlbTargetGroupStates(ycpProfile, ylbId, tgId)
            # Do not try to map ip -> fqdn as there a few ways to write out the same IP address.
            for ip, state in states:
                if state == "HEALTHY":
                    inactive.discard(ip)
                else:
                    inactive.add(ip)

        if inactive.issubset(downtimeFqdns):
            if len(inactive) > 0:
                logging.info("All reals are healthy (except downtimed %s)", ", ".join(inactive))
            else:
                logging.info("All reals are healthy")
            return

        logging.debug("Not all reals are active: %s", inactive)

        time.sleep(BACKOFF)

    raise fail.FailException("Waiting for ylb {0} timed out, still inactive {1}".format(ylbId, ", ".join(inactive)))


def getYlbTargetGroupIds(ycpProfile, ylbId):
    cmd = ["ycp", "load-balancer", "network-load-balancer", "get", "--id", ylbId, "--profile", ycpProfile]
    try:
        out = subprocess.check_output(cmd, stderr=subprocess.PIPE).decode("utf-8")
        result = yaml.safe_load(out)
        ret = []
        for tg in result["attached_target_groups"]:
            ret.append(tg["target_group_id"])
        return ret
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running {0} failed: exit code {1}:\n{2}\n{3}".format(
            cmd, e.returncode, e.output.decode("utf-8"), e.stderr.decode("utf-8")))


def getYlbTargetGroupStates(ycpProfile, ylbId, tgId):
    cmd = ["ycp", "load-balancer", "network-load-balancer", "get-target-states", '-r@', "--profile", ycpProfile]
    try:
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        data = "network_load_balancer_id: \"{0}\"\ntarget_group_id: \"{1}\"".format(ylbId, tgId).encode("utf-8")
        out, _ = p.communicate(data)
        result = yaml.safe_load(out.decode("utf-8"))
        ret = []
        for t in result["target_states"]:
            ret.append((t["address"], t["status"]))
        return ret
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running {0} failed: exit code {1}:\n{2}\n{3}".format(
            cmd, e.returncode, e.output.decode("utf-8"), e.stderr.decode("utf-8")))
