import logging

from . import fail
from . import valuediff


def getImagesFromPodmanifests(manifests):
    ret = set()
    for manifest in manifests:
        if manifest is None:
            continue
        ret.update(getImagesFromPodmanifestImpl(manifest))
    logging.debug("Got images: %s", ", ".join(ret))
    return ret


def getImagesFromPodmanifestImpl(manifest):
    ret = set()
    if manifest["kind"] != "PodList":
        raise fail.FailException("Strange podmanifest, kind is {0}".format(manifest["kind"]))
    for pod in manifest["items"]:
        if pod["kind"] != "Pod":
            raise fail.FailException("Strange podmanifest, pod kind is {0}".format(pod["kind"]))
        spec = pod["spec"]
        if "containers" in spec:
            for c in spec["containers"]:
                ret.add(c["image"])
    return ret


def getContainersToRestart(beforeManifests, afterManifests):
    ret = {}
    for key in afterManifests:
        afterManifest = afterManifests[key]
        if afterManifest is None:
            continue
        if key not in beforeManifests or beforeManifests[key] is None:
            ret[key] = []
            for pod in afterManifest["items"]:
                ret[key].extend(getContainers(pod))
            logging.debug("Container starting on %s: %s", key, ", ".join(ret[key]))
            continue
        ret[key] = getContainersToRestartImpl(beforeManifests[key], afterManifest)
        logging.debug("Container restart on %s: %s", key, ", ".join(ret[key]))
    return ret


def getContainersToRestartImpl(beforeManifest, afterManifest):
    beforePods = {}
    afterPods = {}
    for pod in beforeManifest["items"]:
        metadata = pod["metadata"]
        beforePods[metadata["name"] + "@" + metadata["namespace"]] = pod
    for pod in afterManifest["items"]:
        metadata = pod["metadata"]
        afterPods[metadata["name"] + "@" + metadata["namespace"]] = pod

    ret = []
    for key, pod in afterPods.items():
        if key not in beforePods or hasDiff(beforePods[key], pod):
            ret += getContainers(pod)

    return ret


def hasDiff(value1, value2):
    diff, _ = valuediff.computeDiff(value1, value2, "")
    return len(diff) > 0


def getAllContainers(manifest):
    ret = []
    for pod in manifest["items"]:
        ret += getContainers(pod)
    return ret


def getContainers(pod):
    if "containers" not in pod["spec"]:
        return []
    return [c["name"] for c in pod["spec"]["containers"]]
