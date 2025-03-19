package internal

import (
	"fmt"
	"io/ioutil"
	"time"

	"gopkg.in/yaml.v2"

	"whitey/collector"
	"whitey/push"
)

const (
	defautInterval  uint8  = 30
)

type YamlConfig struct {
	Independent      bool                         `yaml:"independent"`
	PushData         push.Data                    `yaml:"push"`
	Interval         uint8                        `yaml:"interval"`
	CollectorConfigs []collector.YamlCollectorCfg `yaml:"collectors"`
}

func InitApp(cfgFilePath string) (*App, error) {
	configFile, err := ioutil.ReadFile(cfgFilePath)
	if err != nil {
		return nil, fmt.Errorf("Failed to read the config file: %w", err)
	}

	yamlCfg := &YamlConfig{}
	if err := yaml.Unmarshal(configFile, yamlCfg); err != nil {
		return nil, fmt.Errorf("Failed to unmarshall config file: %w", err)
	}

	// Init app configuration parametes
	app := &App{}

	app.Interval = time.Second * time.Duration(defautInterval)
	if yamlCfg.Interval != 0 {
		app.Interval = time.Second * time.Duration(yamlCfg.Interval)
	}

	app.Independent = yamlCfg.Independent

	// Init collectors
	for _, collectorCfg := range yamlCfg.CollectorConfigs {
		c, err := collector.Init(collectorCfg, yamlCfg.PushData, app.Interval)
		if err != nil {
			return nil, err
		}
		app.Collectors = append(app.Collectors, c)
	}


	return app, nil
}
