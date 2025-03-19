package collector

import (
	"fmt"
	"whitey/push"
	"whitey/ua"
)

type BGPStateTask struct {
	PushType  push.Type
	Endpoint  string
	Collector Collector
	Metrics   Metrics
}

func (t *BGPStateTask) SetPushType(pt push.Type) {
	t.PushType = pt
}

func (t *BGPStateTask) SetEndpoint(endpoint string) {
	t.Endpoint = endpoint
}

func (t *BGPStateTask) SetCollector(collector Collector) {
	t.Collector = collector
}

func (t *BGPStateTask) GetMetrics() error {
	switch t.PushType {
	case push.UA:
		peers := t.Collector.GetData().BGPPeers
		t.Metrics.UA = ua.GetBGPStateMetrics(peers)
	default:
		return fmt.Errorf("type does not supported yet: %s", t.PushType.String())
	}

	return nil
}

func (t *BGPStateTask) Push() error {
	switch t.PushType {
	case push.UA:
		err := ua.Push(t.Metrics.UA, t.Endpoint)
		if err != nil {
			return err
		}
	default:
		return fmt.Errorf("type does not supported yet: %s", t.PushType.String())
	}

	return nil
}

type BGPCountersTask struct {
	PushType  push.Type
	Endpoint  string
	Collector Collector
	Metrics   Metrics
}

func (t *BGPCountersTask) SetPushType(pt push.Type) {
	t.PushType = pt
}

func (t *BGPCountersTask) SetEndpoint(endpoint string) {
	t.Endpoint = endpoint
}

func (t *BGPCountersTask) SetCollector(collector Collector) {
	t.Collector = collector
}

func (t *BGPCountersTask) GetMetrics() error {
	switch t.PushType {
	case push.UA:
		peers := t.Collector.GetData().BGPPeers
		t.Metrics.UA = ua.GetBGPCountersMetrics(peers)
	default:
		return fmt.Errorf("type does not supported yet: %s", t.PushType.String())
	}

	return nil
}

func (t *BGPCountersTask) Push() error {
	switch t.PushType {
	case push.UA:
		err := ua.Push(t.Metrics.UA, t.Endpoint)
		if err != nil {
			return err
		}
	default:
		return fmt.Errorf("type does not supported yet: %s", t.PushType.String())
	}

	return nil
}
