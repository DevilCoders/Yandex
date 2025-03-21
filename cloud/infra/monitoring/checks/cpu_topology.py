#!/usr/bin/env python3
"""
This script do next checks:
 - compares /etc/yc/infra/compute_cpu_topology.yaml thread info and generated by /usr/bin/cpu_topology.py script
 - compares /proc/cmdline thread info and generated by /usr/bin/cpu_topology.py script
 - compares /etc/yc/compute-node/config.yaml thread info and generated by /usr/bin/cpu_topology.py script
 - compares /etc/systemd/system.conf thread infoand generated by /usr/bin/cpu_topology.py script
"""
import configparser
import os
import sys
import time
from collections import defaultdict

import yaml
from yc_monitoring import (
    JugglerPassiveCheck,
    JugglerPassiveCheckException,
)

sys.path.append('/usr/bin/')
from isolate_cpu import (
    RequiredResources,
    RESOURCE_TEMPLATES,
    Threads,
)

CPU_TOPOLOGY_FILE = "/etc/yc/infra/compute_cpu_topology.yaml"
COMPUTE_CONFIG_FILE = "/etc/yc/compute-node/config.yaml"
CMDLINE_FILE = "/proc/cmdline"
SYSTEMD_CONFIG_FILE = "/etc/systemd/system.conf"
CACHE_PATH = "/var/tmp/infra_cpu_topology.cache"
CACHE_VALID_TIME_SEC = 24 * 3600  # 24 hours


class CacheExpiredError(Exception):
    pass


class InvalidCacheError(Exception):
    pass


class CpuConfigNotObtainedError(Exception):
    pass


class DuplicateCmdlineParametersError(Exception):
    pass


def read_yaml_cache():
    """
    Read information from cache file
    """
    if os.stat(CACHE_PATH).st_mtime + CACHE_VALID_TIME_SEC < time.time():
        raise CacheExpiredError
    with open(CACHE_PATH) as fd:
        try:
            cache_content = yaml.load(fd.read(), Loader=yaml.SafeLoader)
        except yaml.YAMLError:
            raise InvalidCacheError
    if not cache_content:
        raise InvalidCacheError
    return cache_content


def write_yaml_cache(path, data):
    """
    Write information about disks to cache file
    """
    with open(path, "w+") as fd:
        try:
            write_data = yaml.dump(data)
        except TypeError:
            write_data = None
        fd.write(write_data)


def are_lists_differ(list1, list2):
    """
    Compares two lists and asserts they have same number of elements and same elements
    Length of lists is being compared to ensure, that there are no duplicates in lists
    """
    return len(list1) != len(list2) or set(list1) != set(list2)


def get_cores_info_from_cpu_topology():
    """
    Return info about thread types and cores from cpu_topology file
    Example:
        compute: [5, 45]
        irqaffinity: [0, 40]
        isolcpus: [5, 45]
    """
    try:
        with open(CPU_TOPOLOGY_FILE) as fd:
            local_cpu_config = yaml.load(fd.read(), Loader=yaml.SafeLoader)
        return {thread: local_cpu_config.get(thread, [])
                for thread in Threads.ALL_THREADS
                if thread in local_cpu_config
                }
    except OSError as err:
        raise Exception("File cannot be read: {}".format(err))


def get_cores_info_from_compute_config():
    """
    Return info about thread types and cores from compute config
    Example:
        numa_nodes:
            - system_cores: [0, 40, 1, 41, 2, 42, 3, 43, 4, 44]
              system_memory: 24G
              service_cores: [0, 40, 3, 43, 4, 44]
              shared_cores_limit: 40
    """
    try:
        with open(COMPUTE_CONFIG_FILE) as fd:
            compute_resources = yaml.load(fd.read(), Loader=yaml.SafeLoader)["compute_resources"]["numa_nodes"]
        compute_cpu_config = {numa: compute_resources[numa] for numa in range(len(compute_resources))}
        system_cores = {"node{}_system".format(numa): compute_cpu_config[numa].get("system_cores", [])
                        for numa in compute_cpu_config
                        }
        service_cores = {"node{}_service".format(numa): compute_cpu_config[numa].get("service_cores", [])
                         for numa in compute_cpu_config
                         }
        return {**system_cores, **service_cores}
    except OSError as err:
        raise Exception("File cannot be read: {}".format(err))
    except LookupError as err:
        raise Exception("File cannot be parsed: {}".format(err))


def get_cores_from_systemd_config():
    """
    Return info about wrong value in systemd config
    Example:
        [Manager]
        CPUAffinity=0,40,20,60,21,61,22,62,23,63,24,64,3,43,4,44
    """
    section = "Manager"
    option = "CPUAffinity"
    config = configparser.ConfigParser()
    try:
        config.read(SYSTEMD_CONFIG_FILE)
        for config_section in config.sections():
            if option in config[config_section] and config_section != section:
                raise Exception("Option {} belongs to wrong section: {}".format(option, config_section))
        return {"systemdaffinity": config.get(section, option).split(",")}
    except configparser.NoSectionError:
        raise Exception("Section '{}' not found".format(section))
    except configparser.NoOptionError:
        raise Exception("Option '{}' not found".format(option))
    except configparser.DuplicateOptionError:
        raise Exception("Option '{}' is duplicated in the config".format(option))


def get_cores_info_from_cmdline():
    """
    Return info about thread types and cores from cmdline
    Example:
        isolcpus=1,41,2,42 nohz_full=5,45,25,... irqaffinity=0,40,... ro biosdevname=0
    """
    thread_types = {"isolcpus", "nohz_full", "irqaffinity"}
    cmdline_cores = defaultdict(set)
    with open(CMDLINE_FILE) as fd:
        cmdline = fd.read().strip().split()
        try:
            splitted_cmdline = [(param.split("=")[0], param.split("=")[1]) for param in cmdline if "=" in param]
            for (param, value) in splitted_cmdline:
                if param in thread_types:
                    cmdline_cores[param].add(value)
        except (TypeError, LookupError) as err:
            raise Exception("Parsing error: {}".format(err))
        if any(len(cmdline_cores[key]) > 1 for key in cmdline_cores.keys()):
            raise DuplicateCmdlineParametersError("Found duplicate thread type with multiple values")
    return {"cmdline_{}".format(thread_type): next(iter(cmdline_cores[thread_type])).split(",")
            for thread_type in cmdline_cores.keys()
            }


def check_cpu_topology(check):
    """
    Run checks and output resulting status
    """
    threads = RequiredResources(RESOURCE_TEMPLATES)

    try:
        valid_topology = read_yaml_cache()
    except (CacheExpiredError, InvalidCacheError, OSError):
        valid_topology = yaml.load(threads.generate_compute_cpu_topology(), Loader=yaml.SafeLoader)
    if not valid_topology:
        raise CpuConfigNotObtainedError
    valid_topology.update({
        "cmdline_isolcpus": valid_topology.get("network", []),
        "cmdline_nohz_full": valid_topology.get("compute", []) + valid_topology.get("network", []),
        "cmdline_irqaffinity": valid_topology.get("system", []) + valid_topology.get("kikimr", []) + valid_topology.get(
            "nbs", []),
    })
    write_yaml_cache(CACHE_PATH, valid_topology)

    config_parsers_params = {
        CPU_TOPOLOGY_FILE: {
            "parser": get_cores_info_from_cpu_topology,
            "required_thread_types": {
                "compute",
                "irqaffinity",
                "isolcpus",
                "kikimr",
                "nbs",
                "network",
                "node0_service",
                "node0_system",
                "node1_service",
                "node1_system",
                "system",
                "systemdaffinity"
            },
        },
        COMPUTE_CONFIG_FILE: {
            "parser": get_cores_info_from_compute_config,
            "required_thread_types": {
                "node0_service",
                "node0_system",
                "node1_service",
                "node1_system",
            },
        },
        SYSTEMD_CONFIG_FILE: {
            "parser": get_cores_from_systemd_config,
            "required_thread_types": {
                "systemdaffinity",
            }
        },
        CMDLINE_FILE: {
            "parser": get_cores_info_from_cmdline,
            "required_thread_types": {
                "cmdline_isolcpus",
                "cmdline_nohz_full",
                "cmdline_irqaffinity",
            }
        }
    }
    for config_file, parsing_params in config_parsers_params.items():
        try:
            cores_in_config = parsing_params["parser"]()
            thread_types_with_errors = {thread_type for thread_type in parsing_params["required_thread_types"] if
                                        thread_type not in cores_in_config}

            thread_types_with_errors.update({thread_type
                                             for thread_type, cores in cores_in_config.items()
                                             if are_lists_differ(list(map(int, cores)), valid_topology[thread_type])
                                             })
            if thread_types_with_errors:
                message = "{} file has misconfigured thread types:".format(config_file)
                for thread_type in thread_types_with_errors:
                    message = "{} '{}' has '{}'".format(
                        message,
                        thread_type,
                        cores_in_config[thread_type]
                    )
                check.crit(message)
        except OSError as err:
            check.crit("Cannot read {} file: {}".format(config_file, err))
        except LookupError as err:
            check.crit("{} file cannot be parsed: {}".format(config_file, err))
        except ValueError:
            check.crit("{} file have wrong data".format(config_file))
        except Exception as err:
            check.crit("{} file has errors: {}".format(config_file, err))


def main():
    check = JugglerPassiveCheck("cpu_topology")
    try:
        check_cpu_topology(check)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
