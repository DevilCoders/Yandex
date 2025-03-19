import datetime
import dateutil.parser
import dateutil.tz
import json
import logging
import os
import subprocess
import time

from . import dnsresolve
from . import fail


BACKOFF = 5.0

# Wait 20 minutes for the instance too boot.
WAIT_FOR_SSH = 1200

# The timeout is used only for retrying.
WAIT_FOR_SKM_TIMEOUT = BACKOFF * 3

# Wait 5 minutes for images to download in the regular case.
WAIT_FOR_IMAGES_TIMEOUT = 300
# Wait 15 minutes for fqdn to appear and for images to download if the instance should have been restarted.
WAIT_FOR_IMAGES_LONG_TIMEOUT = 900


# Wait 3.5 minutes for all containers to start. All images should have been pulled by this time.
WAIT_FOR_CONTAINERS_TIMEOUT = 150

# Consider containers to be started if it is running less than RECENT_CONTAINER_TIME.
RECENTLY_STARTED_CONTAINER_TIME = WAIT_FOR_IMAGES_LONG_TIMEOUT * 2

UNIX_START_TIME = datetime.datetime(1970, 1, 1, tzinfo=dateutil.tz.tzutc())

# Wait one minute for envoy to start (after the container has been up).
ENVOY_START_TIMEOUT = 60

# Wait 15 seconds so that we are sure all downstream balancers picked up our healthcheck status.
WAIT_BEFORE_AND_AFTER = 15

NAME_LABEL = "{{.Label \\\"io.kubernetes.container.name\\\"}}"

# Check DNS CONSISTENCY_TIMES and consider the DNS to be consistent if the addresses were the same.
CONSISTENCY_TIMES = 30
CONSISTENCY_BACKOFF = 1
MAX_CONSISTENCY_CHECKS = 50 # Around 25 minutes

# Pssh path can be added to PATH as ~/smth, not as a full path. Expand all PATH entries.


def expandUserPath():
    newPath = [os.path.expanduser(p) for p in os.getenv("PATH").split(":")]
    os.putenv("PATH", ":".join(newPath))


expandUserPath()


def hasPssh():
    try:
        subprocess.check_output(["pssh", "--version"])
        return True
    except subprocess.CalledProcessError:
        return False


def runCommand(fqdn, command, quiet=False):
    logging.debug("Running %s on %s", command, fqdn)
    logLevel = logging.ERROR if not quiet else logging.DEBUG
    try:
        out = subprocess.check_output(["pssh", "run", "--format=json", command, fqdn]).decode("utf-8")
        doc = json.loads(out)
        stdout_text = doc["stdout"] if "stdout" in doc else ""
        stderr_text = doc["stderr"] if "stderr" in doc else ""
        out = stdout_text + stderr_text
        error_text = doc["error"] if "error" in doc else ""
        exit_status = doc["exit_status"] if "exit_status" in doc else 0
        failed = error_text != "" or exit_status != 0
        if failed:
            logging.log(logLevel, "Error running %s: exit code %d, %s, %s", command, exit_status, error_text, out)
        return out, failed
    except subprocess.CalledProcessError as e:
        logging.log(logLevel, "Error running %s: pssh exit code %d, %s",
                    command, e.returncode, e.output.decode("utf-8"))
        return "", True


def waitForSsh(fqdn):
    deadlineTime = time.time() + WAIT_FOR_SSH
    while True:
        _, failed = runCommand(fqdn, "date", True)
        if not failed:
            return

        if time.time() > deadlineTime:
            raise fail.FailException("Waiting for ssh timed out")

        time.sleep(BACKOFF)


def waitForConsistentDns(fqdn):
    for count in range(MAX_CONSISTENCY_CHECKS):
        addrs = set()
        for i in range(CONSISTENCY_TIMES):
            time.sleep(CONSISTENCY_BACKOFF)
            addrs.add(dnsresolve.resolveFqdn(fqdn))
        if len(addrs) == 1:
            return
        if count % 5 == 0:
            logging.info("Still got multiple addresses for %s: %s", fqdn, addrs)
        else:
            logging.info("Still got multiple addresses for %s: %s", fqdn, addrs)



def restartSkm(fqdn):
    deadlineTime = time.time() + WAIT_FOR_SKM_TIMEOUT
    cmd = "sudo systemctl restart skm"
    while time.time() < deadlineTime:
        _, failed = runCommand(fqdn, cmd)
        if not failed:
            return

        time.sleep(BACKOFF)

    raise fail.FailException("Could not restart SKM")


def waitForDockerImages(fqdn, images, hasRestarted):
    timeout = WAIT_FOR_IMAGES_LONG_TIMEOUT if hasRestarted else WAIT_FOR_IMAGES_TIMEOUT
    deadlineTime = time.time() + timeout
    gotImages = set()
    while time.time() < deadlineTime:
        cmd = "for id in `sudo docker images --format \"{{.ID}}\"`; do" \
            + "  sudo docker image inspect $id --format \"{{json .RepoTags}}\";" \
            + "done"
        dockerOut, failed = runCommand(fqdn, cmd)
        if failed:
            continue

        for l in dockerOut.split("\n"):
            if l.strip() == "":
                continue
            try:
                imageTags = json.loads(l)
                gotImages.update(set(imageTags))
            except:
                logging.error("Could not parse tags from docker ps: %s", l)
        if set(images).issubset(gotImages):
            return
        logging.debug("Not all images pulled, sleeping: %s", gotImages)

        time.sleep(BACKOFF)

    raise fail.FailException(
        "Waiting for images timed out after {0} seconds, got only {1}, still missing {2}".format(
            timeout, ", ".join(gotImages), ", ".join(set(images) - gotImages)))


def waitForContainers(fqdn, restartedContainers):
    deadlineTime = time.time() + WAIT_FOR_CONTAINERS_TIMEOUT
    gotContainers = set()
    while time.time() < deadlineTime:
        # Local and remote times may differ, get date from the server.
        dateOut, failed = runCommand(fqdn, "date +%s")
        if failed:
            continue
        now = int(dateOut.strip())

        cmd = "sudo docker ps --format \"" + NAME_LABEL + "\\t{{.CreatedAt}}\""
        dockerOut, failed = runCommand(fqdn, cmd)
        if failed:
            continue

        for l in dockerOut.split("\n"):
            if l.strip() == "":
                continue
            v = l.split("\t")
            if len(v) != 2:
                logging.error("Could not parse tuple from docker ps: '%s'", l)
                continue
            name, createdAtStr = v
            # I really cannot express how bad all this is: Docker returns timestamp in some made-up (by Go? by
            # Docker authors?) format (not ISO), which cannot be parsed by Python2 stdlib at all and requires
            # trimming before it can be parsed by dateutil library.
            #
            # Another little gem: there is no standard way to convert datetime to Unix time in Python O_o
            createdAtStr = createdAtStr.replace("UTC", "").strip()
            createdAt = (dateutil.parser.parse(createdAtStr) -
                         UNIX_START_TIME).total_seconds()
            if now - createdAt < RECENTLY_STARTED_CONTAINER_TIME:
                gotContainers.add(name)

        if set(restartedContainers).issubset(gotContainers):
            return

        logging.debug("Not all containers started, sleeping: %s", gotContainers)
        time.sleep(BACKOFF)

    raise fail.FailException("Waiting for containers to start timed out, got only {0}, still missing {1}".format(
        ", ".join(gotContainers), ", ".join(set(restartedContainers) - gotContainers)))


def prepareContainersBeforeApply(fqdn, containersToRestart, containersDeploySpec):
    startTime = time.time()
    disabledEnvoys = False
    for container in containersDeploySpec:
        if container["name"] in containersToRestart:
            if "is-envoy" in container:
                deadlineTime = startTime + ENVOY_START_TIMEOUT
                disableEnvoy(fqdn, container["admin-port"], deadlineTime)
                disabledEnvoys = True

    if disabledEnvoys:
        logging.info("Waiting a bit before running apply...")
        time.sleep(WAIT_BEFORE_AND_AFTER)


def checkContainersAfterApply(fqdn, containersToRestart, containersDeploySpec):
    for container in containersDeploySpec:
        if container["name"] in containersToRestart:
            checkContainerAfterApply(fqdn, container)

    logging.info("Waiting a bit after running apply...")
    time.sleep(WAIT_BEFORE_AND_AFTER)


def checkContainerAfterApply(fqdn, containerDeploySpec):
    startTime = time.time()
    containerName = containerDeploySpec["name"]
    if "wait-for-log" in containerDeploySpec:
        deadlineTime = startTime + containerDeploySpec["wait-time"]
        waitForContainerLog(fqdn, containerName, containerDeploySpec["wait-for-log"], deadlineTime)
    if "is-envoy" in containerDeploySpec:
        # Re-enable envoy if it has been disabled (e.g. we did not actually restart the envoy and it is still
        # down).
        deadlineTime = startTime + ENVOY_START_TIMEOUT
        enableEnvoy(fqdn, containerDeploySpec["admin-port"], deadlineTime)
        waitHealthcheck(fqdn, containerDeploySpec["healthcheck-port"], deadlineTime)


def waitForContainerLog(fqdn, containerName, stringToFind, deadlineTime):
    logging.info("Waiting for '%s' to appear in container %s logs...",
                 stringToFind, containerName)
    while time.time() < deadlineTime:
        cmd = "for id in `sudo docker ps -a --format \"{{.ID}}\\t{{.CreatedAt}}\" --filter \"label=io.kubernetes.container.name=" + containerName + "\"" \
            + " | sort -k 2  -r  | awk '{print $1}' | head -1`; do" \
            + "  sudo docker logs -t --tail 100000 $id;" \
            + "done"
        dockerOut, failed = runCommand(fqdn, cmd)
        if failed:
            continue

        if stringToFind in dockerOut:
            logging.info("Found '%s' in container logs for %s",
                         stringToFind, containerName)
            return

        logging.debug("Could not find '%s' in container logs for %s, sleeping", stringToFind, containerName)
        time.sleep(BACKOFF)

    raise fail.FailException("Could not find '{0}' in logs for container {1}".format(stringToFind, containerName))


def disableEnvoy(fqdn, adminPort, deadlineTime):
    logging.info("Disabling envoy at %d...", adminPort)
    cmd = "curl -X POST http://localhost:{0}/healthcheck/fail".format(adminPort)
    while time.time() < deadlineTime:
        _, failed = runCommand(fqdn, cmd)
        if not failed:
            logging.info("Disabled envoy")
            return

        time.sleep(BACKOFF)
    # Do nothing if we cannot disable envoy.


def enableEnvoy(fqdn, adminPort, deadlineTime):
    logging.info("Enabling envoy at %d...", adminPort)
    cmd = "curl -X POST http://localhost:{0}/healthcheck/ok".format(adminPort)
    while time.time() < deadlineTime:
        # Be quiet, because Envoy could still be starting.
        _, failed = runCommand(fqdn, cmd, True)
        if not failed:
            logging.info("Enabled envoy")
            return

        time.sleep(BACKOFF)

    raise fail.FailException("Could not disable envoy at port {0}".format(adminPort))


def waitHealthcheck(fqdn, healthcheckPort, deadlineTime):
    logging.info("Waiting for healthcheck at %d...", healthcheckPort)
    cmd = "curl http://localhost:{0}".format(healthcheckPort)
    while time.time() < deadlineTime:
        _, failed = runCommand(fqdn, cmd, True)
        if not failed:
            logging.info("Healthcheck at %d ok", healthcheckPort)
            return

        time.sleep(BACKOFF)

    raise fail.FailException("Could not wait until envoy healthcheck at port {0}".format(healthcheckPort))
