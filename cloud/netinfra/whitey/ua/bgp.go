package ua

import "whitey/bgp"

func GetBGPStateMetrics(peers []bgp.Peer) []Metric {
	metrics := []Metric{}

	for _, peer := range peers {
		stateMetric := Metric{
			Value: peer.State,
			Labels: map[string]string{
				"name":  "state",
				"vrf":   peer.Vrf,
				"group": peer.Group,
				"peer":  peer.Address,
			},
			Type: "DGAUGE",
		}
		metrics = append(metrics, stateMetric)

		uptimeMetric := Metric{
			Value: peer.Uptime,
			Labels: map[string]string{
				"name":  "up_for",
				"vrf":   peer.Vrf,
				"group": peer.Group,
				"peer":  peer.Address,
			},
			Type: "COUNTER",
		}
		metrics = append(metrics, uptimeMetric)
	}

	return metrics
}

func GetBGPCountersMetrics(peers []bgp.Peer) []Metric {
	metrics := []Metric{}

	for _, peer := range peers {
		for _, af := range peer.AFs {
			prefixInMetric := Metric{
				Value: af.Prefix.In,
				Labels: map[string]string{
					"name":  "prefix_in",
					"vrf":   peer.Vrf,
					"group": peer.Group,
					"peer":  peer.Address,
					"af":    af.Name,
				},
				Type: "IGAUGE",
			}
			metrics = append(metrics, prefixInMetric)

			prefixOutMetric := Metric{
				Value: af.Prefix.Out,
				Labels: map[string]string{
					"name":  "prefix_out",
					"vrf":   peer.Vrf,
					"group": peer.Group,
					"peer":  peer.Address,
					"af":    af.Name,
				},
				Type: "IGAUGE",
			}
			metrics = append(metrics, prefixOutMetric)
		}
	}

	return metrics
}
