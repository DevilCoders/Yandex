package collector

import (
	"fmt"
	"time"

	"whitey/push"
)

func Init(cfg YamlCollectorCfg, pushData push.Data, baseInterval time.Duration) (Collector, error) {
	collector, exists := collectorMap[cfg.Type]
	if !exists {
		return nil, fmt.Errorf("Collector type %s does not exist", cfg.Type)
	}

	collector.SetInterval(baseInterval)
	if cfg.Interval != 0 {
		collector.SetInterval(time.Duration(cfg.Interval) * time.Second)
	}

	for kind, endpoint := range cfg.Tasks {
		task := taskMap[kind]
		task.SetPushType(pushData.Type)
		task.SetEndpoint(pushData.Url + endpoint)
		task.SetCollector(collector)

		collector.AddTask(task)
	}

	return collector, nil
}

var collectorMap = map[string]Collector{
	// bgp
	"frr": &FRRCollector{},
}

var taskMap = map[string]Task{
	// bgp
	"state":    &BGPStateTask{},
	"counters": &BGPCountersTask{},
}
