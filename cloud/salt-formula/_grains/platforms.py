"""Generates platforms specification."""

from __future__ import print_function

KILOBYTE = 1024
MEGABYTE = 1024 * KILOBYTE
GIGABYTE = 1024 * MEGABYTE


def generate_platforms(ci_mode=False, prod=False):
    multi_socket_threshold = 4 if ci_mode else 18
    memory_per_core_for_ci = [64 * MEGABYTE] if ci_mode else []

    return [{
        "id": "standard-v1",
        "default": not prod,
        "hardware_platforms": ["xeon-e5-2660"],
        "allowed_configurations": dict({
            5: {
                "cores": {
                    1: [1, 2],
                    2: [4],
                } if ci_mode else {
                    1: [1, 2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    512 * MEGABYTE,
                    GIGABYTE,
                    512 * MEGABYTE + GIGABYTE,
                    2 * GIGABYTE,
                ],
            },
            20: {
                "cores": {
                    1: [1, 2],
                    2: [4],
                } if ci_mode else {
                    1: [1, 2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            100: {
                "cores": {
                    1: [1] + list(range(2, multi_socket_threshold - 1, 2)),
                    2: list(range(multi_socket_threshold, 33, 2)),
                },
                "memory_per_core": memory_per_core_for_ci + [size * GIGABYTE for size in range(1, 9)],
            },
        })
    }, {
        "id": "mdb-v1",
        "internal": True,
        "hardware_platforms": ["xeon-e5-2660"],
        "allowed_configurations": dict({
            5: {
                "cores": {
                    1: [1, 2],
                    2: [4],
                } if ci_mode else {
                    1: [1, 2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    512 * MEGABYTE,
                    GIGABYTE,
                    512 * MEGABYTE + GIGABYTE,
                    2 * GIGABYTE,
                ],
            },
            20: {
                "cores": {1: [2]},
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            50: {
                "cores": {1: [2]},
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            100: {
                "cores": {
                    1: [1] + list(range(2, multi_socket_threshold - 1, 2)),
                    2: list(range(multi_socket_threshold, 33, 2)),
                },
                "memory_per_core": memory_per_core_for_ci + [size * GIGABYTE for size in range(1, 33)],
            },
        })
    }, {
        "id": "standard-v2",
        "default": prod,
        "hardware_platforms": ["xeon-gold-6230"],
        "allowed_configurations": dict({
            5: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [256 * MEGABYTE] + [
                    n * 512 * MEGABYTE for n in range(1, 5)
                ],
            },
            20: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            50: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            100: {
                "cores": {
                    1: list(range(2, multi_socket_threshold, 2)),
                    2: list(range(multi_socket_threshold, 49, 4)),
                } if ci_mode else {
                    1: list(range(2, 17, 2)),
                    2: list(range(20, 49, 4)),
                },
                "memory_per_core": memory_per_core_for_ci + [n * GIGABYTE for n in range(1, 9)],
            },
        })
    }, {
        "id": "mdb-v2",
        "internal": True,
        "hardware_platforms": ["xeon-gold-6230"],
        "allowed_configurations": dict({
            5: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [256 * MEGABYTE] + [
                    n * 512 * MEGABYTE for n in range(1, 5)
                ],
            },
            20: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            50: {
                "cores": {
                    1: [2],
                    2: [multi_socket_threshold],
                } if ci_mode else {
                    1: [2, 4],
                },
                "memory_per_core": memory_per_core_for_ci + [
                    n * 512 * MEGABYTE for n in range(1, 9)
                ],
            },
            100: {
                "cores": {
                    1: list(range(2, multi_socket_threshold, 2)),
                    2: list(range(multi_socket_threshold, 49, 4)),
                } if ci_mode else {
                    1: list(range(2, 17, 2)),
                    2: list(range(20, 49, 4)),
                },
                "memory_per_core": memory_per_core_for_ci + [n * GIGABYTE for n in range(1, 9)],
            },
        })
    }]


def get_platforms():
    return {
        "platforms": {
            "prod":     generate_platforms(prod=True),
            "pre-prod": generate_platforms(),

            "testing":  generate_platforms(),
            "hw-ci":    generate_platforms(),

            "dev":      generate_platforms(ci_mode=True),
        }
    }


if __name__ == "__main__":
    print(get_platforms())
