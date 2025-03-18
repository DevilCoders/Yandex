#!/usr/bin/env python3

import yaqutils.misc_helpers as umisc

from omglib import mc_common
import blender_slices as bs
from proxima_description import ProximaEngineDescription

REVISION_2020 = 8201508
WIKI = "https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/acs/offline-metrics/Current-metrics-and-baskets/"


def generate_blender_metrics_antikessel():
    metrics = []

    for name, slices in bs.blender_slices.items():
        description = "proxima2020-antikessel-wizard без колдунщика {}".format(name)
        impact_metric = ProximaEngineDescription(
            name="proxima2020-antikessel-{}-slices-impact".format(name),
            description=description,
            revision=REVISION_2020,
            depth=10,
            target_slices=slices
        )
        metrics.append(impact_metric)

    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="proxima-wizard-antikessel")
    umisc.configure_logger()

    metrics = generate_blender_metrics_antikessel()
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    main()
