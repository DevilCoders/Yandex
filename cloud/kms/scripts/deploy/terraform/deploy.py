import argparse
import itertools
import json
import logging
import os
import sys
import time
import traceback

from . import colorterm
from . import balancer
from . import fail
from . import instance
from . import podmanifest
from . import playsound
from . import remote
from . import terraform


class ApplyState:
    deploySpec = {}
    withSound = True

    onlyInstances = None
    downtimeInstances = set([])
    changedAddresses = []
    allAddresses = []
    dockerImages = set([])
    instanceActions = {}
    containersToRestart = set([])
    ycpProfile = None
    ylbIds = []
    updatedSkm = set([])
    skipInitialCheck = False


startTime = time.time()


def playSound(state):
    # Playing sounds immediately after running the script (e.g. due to terraform fail) is super-annoying.
    if state.withSound and time.time() > startTime + 10:
        playsound.play("ding.wav")


def areSimilarDiffPaths(path1, path2):
    # HACK: When displaying the text file diff, do not output several lines with the same diff path,
    # which happens when a number of lines got added or deleted.
    if path1 is None or path2 is None:
        return False
    if path1 == path2:
        return True
    if path1[:-1] == path2[:-1] and type(path1[-1]) == int and type(path2[-1]) == int and abs(path2[-1] - path1[-1]) == 1:
        return True
    return False


def printDiff(diff):
    # HACK: Save the order of changes in this map, use it later to sort the changes. This is very important
    # when displaying the changes in text files when there may be several changes with the same path (which is
    # filename.line_number).
    changeOrder = {}

    groupByChange = {}
    for fqdn in diff:
        for i, e in enumerate(diff[fqdn]):
            changeOrder[e] = i
            if e in groupByChange:
                groupByChange[e].append(fqdn)
            else:
                groupByChange[e] = [fqdn]

    groupByFqdnGroup = {}
    for e in groupByChange:
        fqdns = ", ".join(sorted(groupByChange[e]))
        if fqdns in groupByFqdnGroup:
            groupByFqdnGroup[fqdns].append(e)
        else:
            groupByFqdnGroup[fqdns] = [e]

    sortedFqdns = sorted(groupByFqdnGroup.keys(), key=lambda x: (-len(x), x))
    for fqdns in sortedFqdns:
        # See above for the changeOrder hack.
        sortedChanges = sorted(groupByFqdnGroup[fqdns], key=lambda e: (e[0], changeOrder[e]))
        d = ""
        lastPath = None
        for path, f, t in sortedChanges:
            if not areSimilarDiffPaths(path, lastPath):
                d += ".".join(str(i) for i in path) + ":\n"
            lastPath = path
            if f is not None:
                d += "  - " + colorterm.red(str(f)) + "\n"
            if t is not None:
                d += "  + " + colorterm.green(str(t)) + "\n"
        logging.info("Diff for %s:\n%s", colorterm.yellow(fqdns), d)


def confirmApply(defaultConfirm):
    try:
        answer = input("Apply changes {0}? ".format("[Y/n]" if defaultConfirm else "[y/N]"))
    except KeyboardInterrupt:
        logging.info("Aborting on user request")
        sys.exit(1)
    accepted = ["", "y", "yes"] if defaultConfirm else ["y", "yes"]
    if not answer.lower() in accepted:
        logging.info("Aborting on user request")
        sys.exit(1)


def applyInstances(state):
    logging.info("")
    fqdns = [a[1] for a in state.allAddresses]

    logging.info("Running initial balancer check...")
    if "balancers" in state.deploySpec and not state.skipInitialCheck:
        balancer.checkBalancers(state.deploySpec["balancers"], fqdns, state.ycpProfile, state.ylbIds,
                                state.downtimeInstances)
    logging.info("Initial balancer done")

    for address, fqdn in state.changedAddresses:
        if state.onlyInstances is not None and fqdn not in state.onlyInstances:
            logging.info("Skipping %s on user request", colorterm.yellow(fqdn))
            continue
        if fqdn in state.downtimeInstances:
            logging.info("Skipping %s due to downtime", colorterm.yellow(fqdn))
            continue

        try:
            logging.info("Applying %s...", colorterm.yellow(fqdn))

            if "containers" in state.deploySpec and (fqdn not in state.instanceActions or state.instanceActions[fqdn] != "create"):
                remote.prepareContainersBeforeApply(
                    fqdn, state.containersToRestart[fqdn], state.deploySpec["containers"])

            logging.info("Applyng...")
            terraform.runApply(address)

            hasRestarted = fqdn in state.instanceActions
            if hasRestarted:
                logging.info("Waiting for ssh...")
                remote.waitForSsh(fqdn)
                logging.info("Ssh successful")

                logging.info("Waiting for consistent dns...")
                remote.waitForConsistentDns(fqdn)
                logging.info("DNS is consistent")

            if fqdn in state.updatedSkm:
                logging.info("Restarting SKM after update...")
                remote.restartSkm(fqdn)
                logging.info("SKM restarted")

            if len(state.dockerImages) > 0 and len(state.containersToRestart[fqdn]) > 0:
                logging.info("Waiting for images to be pulled...")
                remote.waitForDockerImages(fqdn, state.dockerImages, hasRestarted)
                logging.info("All images pulled")

            if len(state.containersToRestart[fqdn]) > 0:
                logging.info("Waiting for containers to start...")
                remote.waitForContainers(fqdn, state.containersToRestart[fqdn])
                logging.info("All containers started")

            if "containers" in state.deploySpec:
                remote.checkContainersAfterApply(fqdn, state.containersToRestart[fqdn], state.deploySpec["containers"])

            if "balancers" in state.deploySpec:
                balancer.checkBalancers(state.deploySpec["balancers"], fqdns, state.ycpProfile, state.ylbIds,
                                        state.downtimeInstances)

        except fail.FailException as e:
            logging.error("Error during applying %s, aborting: %s", fqdn, e.message)
            playSound(state)
            sys.exit(1)
        except Exception:
            traceback.print_exc()
            playSound(state)
            sys.exit(1)
        logging.info("Finished applying %s\n", colorterm.yellow(fqdn))

    terraform.storeCachedPlan()
    logging.info(colorterm.green("All done!"))
    playSound(state)


def validateSpec(manifests, deploySpec):
    if "containers" not in deploySpec:
        return
    for _, manifest in manifests.items():
        containers = podmanifest.getAllContainers(manifest)
        if "containers" in deploySpec:
            for c in deploySpec["containers"]:
                if "name" not in c:
                    logging.error("No 'name' in container %s", c)
                    sys.exit(1)
                if c["name"] not in containers:
                    logging.error("Invalid deploy spec, unknown container %s", colorterm.yellow(c))
                    sys.exit(1)
                if "wait-for-log" in c:
                    if "wait-time" not in c:
                        logging.error("No 'wait-time' in container %s", c)
                        sys.exit(1)
                if "is-envoy" in c:
                    if "admin-port" not in c or "healthcheck-port" not in c:
                        logging.error("No 'admin-port' or 'healthcheck' in container %s", c)
                        sys.exit(1)
        if "balancers" in deploySpec:
            for b in deploySpec["balancers"]:
                if "type" not in b:
                    logging.error("No 'type' in balancer %s", b)
                    sys.exit(1)
                if b["type"] == "l3tt":
                    if "service-id" not in b:
                        logging.error("No 'service-id' in balancer %s", c)
                        sys.exit(1)
                elif b["type"] == "ylb":
                    fromTerraform = "from-terraform" in b and b["from-terraform"]
                    hasYlbId = "ylb-id" in b
                    if not fromTerraform and not hasYlbId:
                        logging.error("Either 'ylb-id' or 'from-terraform' must be set in balancer %s", b)
                        sys.exit(1)
                    if fromTerraform and hasYlbId:
                        logging.error("Both 'ylb-id' and 'from-terraform' are set in balancer %s", b)
                        sys.exit(1)


def checkBinaries():
    if not terraform.hasTerraform():
        logging.error("Terraform 0.14 must be installed")
        sys.exit(1)
    if not remote.hasPssh():
        logging.error("pssh must be installed")
        sys.exit(1)
    if not balancer.hasYcp():
        logging.error("Ycp must be installed")
        sys.exit(1)


def applyYlb(args):
    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.preparePlan()
        diff = terraform.getYlbDiffFromPlan(plan)
        for address in diff:
            printDiff({address: diff[address]})
        confirmApply(False)
        logging.info("Applying...")
        terraform.runApplies(diff.keys())
        terraform.storeCachedPlan()
        logging.info("All done!")

    except fail.FailException as e:
        logging.error("Error, aborting: %s", e.message)
        sys.exit(1)

def applyDns(args):
    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.preparePlan()
        diff = terraform.getDnsDiffFromPlan(plan)
        for address in diff:
            printDiff({address: diff[address]})
        confirmApply(False)
        logging.info("Applying...")
        terraform.runApplies(diff.keys())
        terraform.storeCachedPlan()
        logging.info("All done!")

    except fail.FailException as e:
        logging.error("Error, aborting: %s", e.message)
        sys.exit(1)


def ylbStatus(args):
    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.loadOrPreparePlan()
        ycpProfile = terraform.getYcpProfileFromPlan(plan)
        ylbs = terraform.getYlbsFromPlan(plan)
        for ylbId in ylbs:
            listener, tgIds = ylbs[ylbId]
            for tgId in tgIds:
                state = "\n".join(" - {0}: {1}".format(terraform.getFqdnByIpFromPlan(plan, ip), s)
                                  for ip, s in balancer.getYlbTargetGroupStates(ycpProfile, ylbId, tgId))
                logging.info("State for %s (%s):\n%s", listener, tgId, state)

    except fail.FailException as e:
        logging.error("Error, aborting: %s", e.message)
        sys.exit(1)


def processInstances(fqdns, args, callback):
    if args.deploySpec is not None and args.deploySpec != "":
        with open(args.deploySpec) as f:
            deploySpec = json.load(f)
    else:
        deploySpec = {}

    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.loadOrPreparePlan()
        manifestsBefore, _ = terraform.getPodmanifestsFromPlan(plan, False)
        ycpProfile = terraform.getYcpProfileFromPlan(plan)
        allFqdns = [c[1] for c in terraform.getInstanceAddressesFromPlan(plan, False)]
        ylbIds = terraform.getYlbsFromPlan(plan).keys()
        if fqdns == ["all"]:
            fqdns = allFqdns
        if ycpProfile is None:
            raise fail.FailException("ycp_profile variable must be set in terraform")
        for fqdn in fqdns:
            computeId = terraform.findInstanceComputeIdFromPlan(plan, fqdn)
            containers = podmanifest.getAllContainers(manifestsBefore[fqdn])
            if "containers" in deploySpec:
                remote.prepareContainersBeforeApply(fqdn, containers, deploySpec["containers"])
            callback(fqdn, computeId, ycpProfile, deploySpec, containers, allFqdns, ylbIds)

    except fail.FailException as e:
        logging.error("Error during processing instances, aborting: %s", e.message)
        sys.exit(1)


def listInstances(args):
    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.loadOrPreparePlan()
        logging.info("Instances:\n" + "\n".join(" - " + i for i in terraform.listInstancesFromPlan(plan)))

    except fail.FailException as e:
        logging.error("Error during listing instances, aborting: %s", e.message)
        sys.exit(1)


def deleteInstances(args):
    def callback(fqdn, computeId, ycpProfile, deploySpec, containers, allFqdns, ylbIds):
        logging.info("Deleting instance %s...", fqdn)
        instance.deleteInstance(computeId, ycpProfile)
        logging.info("Deleted instance %s", fqdn)
    processInstances(args.deleteInstances.split(","), args, callback)


def rebootInstances(args):
    if args.downtimeInstances is not None:
        downtimeInstances = set(args.downtimeInstances.split(","))
    else:
        downtimeInstances = set([])
    def callback(fqdn, computeId, ycpProfile, deploySpec, containers, allFqdns, ylbIds):
        logging.info("Rebooting instance %s...", fqdn)
        instance.rebootInstance(computeId, ycpProfile)
        logging.info("Rebooted instance %s", fqdn)
        logging.info("Waiting for ssh...")
        remote.waitForSsh(fqdn)
        logging.info("Ssh successful")

        logging.info("Waiting for containers to start...")
        remote.waitForContainers(fqdn, containers)
        logging.info("All containers started")

        if "containers" in deploySpec:
            remote.checkContainersAfterApply(fqdn, containers, deploySpec["containers"])
        if "balancers" in deploySpec:
            balancer.checkBalancers(deploySpec["balancers"], allFqdns, ycpProfile, ylbIds, downtimeInstances)
        logging.info("Instance %s started successfully\n", fqdn)

    processInstances(args.rebootInstances.split(","), args, callback)
    logging.info("All done")


def splitInstanceActions(instanceActions):
    toCreate = []
    toRestart = []
    for i in instanceActions:
        if instanceActions[i] == "create":
            toCreate.append(i)
        elif instanceActions[i] == "restart":
            toRestart.append(i)
        else:
            logging.error("Unknown action %s for %s", instanceActions[i], i)
            sys.exit(1)
    return toCreate, toRestart


def fixContainersToRestart(state, manifestsAfter):
    for i in state.instanceActions:
        if state.instanceActions[i] == "restart":
            # If instance gets restarted, all containers will get restarted.
            state.containersToRestart[i] = podmanifest.getAllContainers(manifestsAfter[i])


def prepare(args):
    state = ApplyState()

    if args.noSound:
        state.withSound = False

    if args.deploySpec is not None and args.deploySpec != "":
        with open(args.deploySpec) as f:
            state.deploySpec = json.load(f)

    if args.onlyInstances is not None:
        state.onlyInstances = set(args.onlyInstances.split(","))

    if args.downtimeInstances is not None:
        state.downtimeInstances = set(args.downtimeInstances.split(","))

    state.skipInitialCheck = args.skipInitialCheck

    os.chdir(args.terraformDir)
    try:
        logging.info("Preparing terraform plan...")
        plan = terraform.preparePlan()
        logging.info("Computing diff from terraform plan...")
        diff = terraform.getInstanceDiffFromPlan(plan, args.ignoreOtherDiff)
        if len(diff) == 0:
            logging.info(colorterm.green("All changes applied, nothing to do"))
            sys.exit(0)

        printDiff(diff)
        manifestsBefore, manifestsAfter = terraform.getPodmanifestsFromPlan(plan, True)
        validateSpec(manifestsAfter, state.deploySpec)
        state.dockerImages = podmanifest.getImagesFromPodmanifests(manifestsAfter.values())
        state.containersToRestart = podmanifest.getContainersToRestart(manifestsBefore, manifestsAfter)
        state.instanceActions = terraform.getInstanceActionsFromPlan(plan)
        fixContainersToRestart(state, manifestsAfter)
        state.updatedSkm = terraform.updatedSkmInPlan(plan)

        defaultApply = True
        instancesToCreate, instancesToRestart = splitInstanceActions(state.instanceActions)
        if len(instancesToCreate) > 0:
            defaultApply = False
            logging.info("Instances to be created:\n" + "\n".join(" - " + colorterm.yellow(i)
                                                                  for i in instancesToCreate))
        if len(instancesToRestart) > 0:
            defaultApply = False
            logging.info("Instances to be restarted:\n" + "\n".join(" - " + colorterm.yellow(i)
                                                                    for i in instancesToRestart))

        containerSet = sorted(set(itertools.chain.from_iterable(state.containersToRestart.values())))
        logging.info("Containers to be restarted:\n%s", "\n".join(" - " + colorterm.yellow(i) for i in containerSet))
    except fail.FailException as e:
        logging.error("Error during preparing, aborting: %s", e.message)
        sys.exit(1)
    except Exception:
        traceback.print_exc()
        sys.exit(1)

    state.changedAddresses = terraform.getInstanceAddressesFromPlan(plan, True)
    state.allAddresses = terraform.getInstanceAddressesFromPlan(plan, False)

    state.ycpProfile = terraform.getYcpProfileFromPlan(plan)
    state.ylbIds = terraform.getYlbsFromPlan(plan).keys()
    if len(state.ylbIds) > 0 and state.ycpProfile is None:
        logging.error("Error: ycp_profile not specified, but ylb balancers are present")
        sys.exit(1)

    confirmApply(defaultApply)
    return state


def main():
    logging.basicConfig(format="%(asctime)s: %(message)s", style="%", level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument("--list", action="store_true", help="list instances, do not run the rest of the deploy")
    parser.add_argument("--delete", dest="deleteInstances", help="delete instance, do not run the rest of the deploy")
    parser.add_argument("--reboot", dest="rebootInstances", help="reboot instance, do not run the rest of the deploy")
    parser.add_argument("--apply-ylb", dest="applyYlb", action="store_true", help="apply changes to load balancer")
    parser.add_argument("--apply-dns", dest="applyDns", action="store_true", help="apply changes to DNS")
    parser.add_argument("--ylb-status", dest="ylbStatus", action="store_true")
    parser.add_argument("--allow-delete", dest="allowDelete", action="store_true")

    parser.add_argument("--only", dest="onlyInstances", help="only the given instnances will be deployed")
    parser.add_argument("--downtime", dest="downtimeInstances", help="do not deploy the given instances and ignore their balancer status")
    parser.add_argument("--ignore-other-diff", dest="ignoreOtherDiff", action="store_true", help="ignore terraform diff outside of instances e.g. in load balancers")
    parser.add_argument("--skip-initial-check", dest="skipInitialCheck", action="store_true", help="skip initial balancer check before deployment")

    parser.add_argument("--terraform-dir", dest="terraformDir", help="directory with terraform files (main.tf)")
    parser.add_argument("--deploy-spec", dest="deploySpec", help="deploy specification")
    parser.add_argument("--debug", dest="debug", action="store_true", help="enable additional logging")
    parser.add_argument("--no-sound", dest="noSound", action="store_true", help="disable sound effects")

    args = parser.parse_args()
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    terraform.allowDelete = args.allowDelete

    checkBinaries()
    if args.applyYlb:
        applyYlb(args)
    elif args.applyDns:
        applyDns(args)
    elif args.ylbStatus:
        ylbStatus(args)
    elif args.list:
        listInstances(args)
    elif args.deleteInstances is not None:
        deleteInstances(args)
    elif args.rebootInstances is not None:
        rebootInstances(args)
    else:
        state = prepare(args)
        applyInstances(state)


if __name__ == "__main__":
    main()
