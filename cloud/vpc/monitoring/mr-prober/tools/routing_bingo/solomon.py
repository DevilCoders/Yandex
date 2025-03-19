import dataclasses
import datetime
import enum
import logging
import os
import urllib.parse
from typing import Dict, List, Optional, Tuple

import requests
from pydantic import BaseModel

from tools.routing_bingo.config import SolomonConfig

_MATRIX_PARAM_PREFIX = "matrix_"


class ProberStatus(enum.Enum):
    NODATA = "nodata"
    SUCCESS = "success"
    FAIL = "fail"


@dataclasses.dataclass
class SolomonProberResult:
    timestamp: Optional[datetime.datetime]
    status: ProberStatus


@dataclasses.dataclass
class SolomonProberResultVector:
    prober_slug: str
    params: Dict[str, str]
    results: List[SolomonProberResult]

    def unsuccessful_results(self) -> List[SolomonProberResult]:
        return [
            result for result in self.results
            if result.status != ProberStatus.SUCCESS
        ]


class SolomonTimeseries(BaseModel):
    class Data(BaseModel):
        kind: str
        type: str

        labels: Dict[str, str]
        alias: str

        timestamps: List[int] = []
        values: List[float] = []

    timeseries: Data


class SolomonResult(BaseModel):
    vector: List[SolomonTimeseries]


class SolomonClient:
    def __init__(self, cfg: SolomonConfig, oauth_token: str):
        self.cfg = cfg
        self._oauth_token = oauth_token

    def get_prober_results(self, time_frame: Tuple[float, float], extra_labels: Dict[str, str],
                           prober_slugs: List[str], result_count: int) -> List[SolomonProberResultVector]:
        metrics = self.get_last_metrics(time_frame, {
            "prober_slug": "|".join(prober_slugs),
            "metric": "success|fail",
            **extra_labels,
        })

        results = {}
        for ts in metrics.vector:
            ts: SolomonTimeseries.Data = ts.timeseries

            slug = ts.labels["prober_slug"]
            params = {key[len(_MATRIX_PARAM_PREFIX):]: value
                      for key, value in ts.labels.items()
                      if key.startswith(_MATRIX_PARAM_PREFIX)}

            value_zip = list(zip(ts.timestamps, ts.values))
            if len(value_zip) < result_count:
                # Note that this could be an outdated run with different set of parameters which wasn't evicted by
                # Solomon TTL. For now we do not check that all matrix variables are present in labels
                logging.warning(f"Unexpected number of values for prober slug='{slug}', params={params}:"
                                f" expected: {result_count}, got {len(value_zip)}")
                continue

            key = (slug, tuple(params.items()))
            if key not in results:
                nodata_results = self._build_nodata_results(time_frame, result_count)
                results[key] = vector = SolomonProberResultVector(slug, params, nodata_results)
            else:
                vector = results[key]

            # truncate outdated result if there is any
            value_zip = value_zip[-result_count:]
            status = ProberStatus(ts.labels["metric"])
            for idx, (timestamp, value) in enumerate(value_zip):
                # We should get 1.0 either for 'fail' or for 'success' timeseries
                if value < 1.0:
                    continue

                dt = datetime.datetime.utcfromtimestamp(timestamp / 1000)
                result = vector.results[idx]
                if result.status == ProberStatus.NODATA:
                    vector.results[idx] = SolomonProberResult(dt, status)
                else:
                    logging.warning(f"Unexpected duplicate status for prober slug='{slug}', params={params}")

        return list(results.values())

    def _build_nodata_results(self, time_frame: Tuple[float, float], result_count: int) -> List[SolomonProberResult]:
        step = (time_frame[1] - time_frame[0]) / result_count
        return [SolomonProberResult(datetime.datetime.utcfromtimestamp(time_frame[0] + (step * (idx + 0.5))),
                                    ProberStatus.NODATA) for idx in range(result_count)]

    def get_last_metrics(self, time_frame: Tuple[float, float], labels: Dict[str, str]) -> SolomonResult:
        """Returns raw metrics"""
        program = self._create_selector({
            "project": self.cfg.project,
            "cluster": self.cfg.cluster,
            "service": self.cfg.service,
            **labels,
        })

        url = urllib.parse.urljoin(self.cfg.endpoint, f"/api/v2/projects/{self.cfg.project}/sensors/data")
        req = {
            "from": int(time_frame[0] * 1000),
            "to": int(time_frame[1] * 1000),
            "program": program,
            "downsampling": {
                "disabled": True,
            }
        }
        logging.debug(f"Fetching data from solomon with req={req}")

        r = requests.post(url, json=req, headers={
            "Content-Type": "application/json",
            "Authorization": f"OAuth {self._oauth_token}",
        })
        return SolomonResult.parse_obj(r.json())

    def _create_selector(self, labels: Dict[str, str]) -> str:
        selectors = [f"{key}='{value}'" for key, value in labels.items()]
        return "{" + ", ".join(selectors) + "}"


def get_solomon_client(cfg: SolomonConfig) -> SolomonClient:
    oauth_token = os.environ.get("SOLOMON_OAUTH_TOKEN")
    if not oauth_token:
        raise RuntimeError("SOLOMON_OAUTH_TOKEN environment variable is not specified."
                           " Check out https://nda.ya.ru/t/n0kcBmt84tFWSq to receive OAuth token.")

    return SolomonClient(cfg, oauth_token)
