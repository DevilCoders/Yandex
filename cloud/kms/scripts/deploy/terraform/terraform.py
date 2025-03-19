import json
import logging
import subprocess
import tempfile
import yaml

from . import fail
from . import tokens
from . import valuediff

allowDelete = False

def hasTerraform():
    try:
        out = subprocess.check_output(["terraform", "version"]).decode("utf-8")
        return out.startswith("Terraform v0.14.")
    except Exception as e:
        logging.debug("Caught exception when running 'terraform version': %s", e)
        return False


def runTerraform(args):
    cmd = ["terraform"] + args
    strArgs = " ".join(args)
    try:
        logging.debug("Running terraform %s", strArgs)
        ret = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode("utf-8")
        logging.debug("Success")
        return ret
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running terraform {0} failed: exit code {1}:\n{2}".format(
            strArgs, e.returncode, e.output.decode("utf-8")))


# HACK: Determine if the terraform requires yc token by reading the variables.tf
def terraformNeedsYcToken():
    with open("variables.tf") as f:
        return "\nvariable \"yc_token\"" in f.read()


def runTerraformWithVars(args):
    cmd = ["terraform"] + args + ["-var", "yandex_token=" + tokens.getYavToken()]
    if terraformNeedsYcToken():
        cmd += ["-var", "yc_token=" + tokens.getYcToken()]
    strArgs = " ".join(args)
    try:
        logging.debug("Running terraform %s", strArgs)
        ret = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode("utf-8")
        logging.debug("Success")
        return ret
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running terraform {0} failed: exit code {1}:\n{2}".format(
            strArgs, e.returncode, e.output.decode("utf-8")))


cachedPlanFile = ".terraform/cached.plan.json"

def preparePlan():
    tf = tempfile.NamedTemporaryFile()
    runTerraformWithVars(["plan", "-input=false", "-out=" + tf.name])
    jsonPlan = runTerraform(["show", "-json", tf.name])
    ret = json.loads(jsonPlan)
    tf.close()
    return ret


def loadOrPreparePlan():
    try:
        with open(cachedPlanFile, "r") as f:
            ret = json.load(f)
            logging.info("Loaded cached plan")
            return ret
    except IOError:
        pass
    return storeCachedPlan()


def storeCachedPlan():
    plan = preparePlan()
    with open(cachedPlanFile, "w") as f:
        json.dump(plan, f)
        logging.info("Stored cached plan")
    return plan


def getInstanceDiffFromPlan(plan, ignoreOtherDiff):
    ret = {}
    for d in plan["resource_changes"]:
        address = d["address"]
        change = d["change"]
        if change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            if d["type"] == "random_string":
                logging.info("%s changed, applying", address)
                runApply(address)
                continue
            if ignoreOtherDiff:
                logging.info("%s changed, ignoring", address)
                continue
            else:
                raise fail.FailException("{0} changed, aborting".format(address))
        actions = change["actions"]
        if actions != ["update"] and actions != ["delete", "create"] and actions != ["create"] \
                and (actions != ["delete"] or not allowDelete):
            raise fail.FailException("Strange change actions for {0}, aborting: {1}".format(address, change["actions"]))
        if change["after"] is not None and "fqdn" in change["after"]:
            key = change["after"]["fqdn"]
        else:
            key = address

        ret[key] = valuediff.computeMapDiff(change["before"], change["after"], "")[0]
    return ret


def getYlbDiffFromPlan(plan):
    ret = {}
    for d in plan["resource_changes"]:
        address = d["address"]
        change = d["change"]
        if change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycp_load_balancer_target_group", "ycp_load_balancer_network_load_balancer"]:
            continue
        actions = change["actions"]
        if actions != ["update"] and actions != ["delete", "create"] and actions != ["create"] \
                and (actions != ["delete"] or not allowDelete):
            raise fail.FailException("Strange change actions for {0}, aborting: {1}".format(address, change["actions"]))
        ret[address] = valuediff.computeMapDiff(change["before"], change["after"], "")[0]
    return ret


def getDnsDiffFromPlan(plan):
    ret = {}
    for d in plan["resource_changes"]:
        address = d["address"]
        change = d["change"]
        if change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycp_dns_dns_zone", "ycp_dns_dns_record_set", "ycp_vpc_inner_address"]:
            continue
        actions = change["actions"]
        if actions != ["update"] and actions != ["delete", "create"] and actions != ["create"] \
                and (actions != ["delete"] or not allowDelete):
            raise fail.FailException("Strange change actions for {0}, aborting: {1}".format(address, change["actions"]))
        ret[address] = valuediff.computeMapDiff(change["before"], change["after"], "")[0]
    return ret


def getPodmanifestsFromPlan(plan, onlyChanged):
    beforeManifests = {}
    afterManifests = {}
    for d in plan["resource_changes"]:
        address = d["address"]
        change = d["change"]
        if onlyChanged and change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        before = change["before"]
        after = change["after"]
        if "metadata" not in after or "podmanifest" not in after["metadata"]:
            raise fail.FailException(
                "no podmanifest and/or metadata in {0}, aborting".format(address))
        fqdn = change["after"]["fqdn"]
        beforeManifests[fqdn] = yaml.safe_load(before["metadata"]["podmanifest"]) if before is not None else None
        afterManifests[fqdn] = yaml.safe_load(after["metadata"]["podmanifest"]) if after is not None else None
    return beforeManifests, afterManifests


def updatedSkmInPlan(plan):
    ret = set()
    for d in plan["resource_changes"]:
        change = d["change"]
        if change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        before = change["before"]
        after = change["after"]
        if before is None or after is None:
            continue
        if "metadata" not in before or "skm" not in before["metadata"] or "metadata" not in after or "skm" not in after["metadata"]:
            continue
        if before["metadata"]["skm"] == after["metadata"]["skm"]:
            continue
        ret.add(change["after"]["fqdn"])

    return ret


def getInstanceAddressesFromPlan(plan, changed):
    ret = []
    for d in plan["resource_changes"]:
        address = d["address"]
        change = d["change"]
        if changed and change["actions"] == ["no-op"]:
            continue
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        ret.append((address, change["after"]["fqdn"]))
    return ret


def getInstanceActionsFromPlan(plan):
    ret = {}
    for d in plan["resource_changes"]:
        change = d["change"]
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        actions = change["actions"]
        fqdn = change["after"]["fqdn"]
        if actions == ["delete", "create"]:
            ret[fqdn] = "restart"
        elif actions == ["create"]:
            ret[fqdn] = "create"
    return ret


def resolveExpressionFromPlan(plan, expr):
    if "constant_value" in expr:
        return expr["constant_value"]

    if "references" not in expr or len(expr["references"]) != 1:
        logging.error("Could not interpret expression: %s", expr)
        return None
    e = expr["references"][0].split(".")
    if e[0] == "var":
        varName = e[1]
        if varName not in plan["variables"]:
            logging.error("Could not interpret expression: %s, no variable %s", e, varName)
            return None
        return plan["variables"][varName]["value"]
    elif e[0] == "module":
        moduleName = e[1]
        varName = e[2]
        if moduleName not in plan["configuration"]["root_module"]["module_calls"]:
            logging.error("Could not interpret expression: %s, no module %s", e, moduleName)
            return None
        module = plan["configuration"]["root_module"]["module_calls"][moduleName]["module"]
        if varName not in module["variables"]:
            logging.error("Could not interpret expression: %s, no variable %s in module %s", e, varName, moduleName)
            return None
        return module["variables"][varName]["default"]
    else:
        logging.error("Could not interpret expression: %s", e)
        return None


def getYcpProfileFromPlan(plan):
    if "ycp" not in plan["configuration"]["provider_config"]:
        return None
    expressions = plan["configuration"]["provider_config"]["ycp"]["expressions"]
    if "ycp_profile" in expressions:
        ret = resolveExpressionFromPlan(plan, expressions["ycp_profile"])
    elif "prod" in expressions:
        ret = "prod" if resolveExpressionFromPlan(plan, expressions["prod"]) else "preprod"
    else:
        ret = None
    logging.info("Autodetected ycp profile %s", ret)
    return ret


def getYlbsFromPlan(plan):
    ret = {}
    for d in plan["resource_changes"]:
        change = d["change"]
        if d["type"] != "ycp_load_balancer_network_load_balancer":
            continue
        if "id" not in change["after"]:
            continue
        ylbId = change["after"]["id"]
        listener = ", ".join("[" + l["external_address_spec"][0]["address"] + "]:" + str(l["port"]) for l in change["after"]["listener_spec"])
        tgIds = [t["target_group_id"] for t in change["after"]["attached_target_group"]]
        ret[ylbId] = (listener, tgIds)
    return ret


def getFqdnByIpFromPlan(plan, ip):
    for d in plan["resource_changes"]:
        change = d["change"]
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        for n in change["after"]["network_interface"]:
            for i in n["primary_v4_address"]:
                if i["address"] == ip:
                    return change["after"]["fqdn"]
            for i in n["primary_v6_address"]:
                if i["address"] == ip:
                    return change["after"]["fqdn"]
    return ip


def listInstancesFromPlan(plan):
    ret = set([])
    for d in plan["resource_changes"]:
        change = d["change"]
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        if change["before"] is not None and "fqdn" in change["before"]:
            ret.add(change["before"]["fqdn"])
        if change["after"] is not None and "fqdn" in change["after"]:
            ret.add(change["after"]["fqdn"])
    return sorted(list(ret))


def findInstanceComputeIdFromPlan(plan, fqdn):
    ret = set([])
    for d in plan["resource_changes"]:
        change = d["change"]
        if d["type"] not in ["ycunderlay_compute_instance", "ycp_compute_instance"]:
            continue
        if change["before"] is not None and "fqdn" in change["before"] and change["before"]["fqdn"] == fqdn:
            if "id" in change["before"]:
                ret.add(change["before"]["id"])
        if change["after"] is not None and "fqdn" in change["after"] and change["after"]["fqdn"] == fqdn:
            if "id" in change["after"]:
                ret.add(change["after"]["id"])

    if len(ret) == 0:
        raise fail.FailException("Could not find instance {0} in terraform".format(fqdn))
    elif len(ret) > 1:
        raise fail.FailException("More than instance {0} found in terraform: {1}".format(fqdn, ", ".join(ret)))
    else:
        return list(ret)[0]


def runApply(address):
    runTerraformWithVars(["apply", "-auto-approve", "-target=" + address])


def runApplies(addresses):
    if len(addresses) > 0:
        runTerraformWithVars(["apply", "-auto-approve"] + ["-target=" + a for a in addresses])
    else:
        logging.info("No changes to apply")
