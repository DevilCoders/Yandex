package solomon

import (
	"errors"
	"io/ioutil"

	"gopkg.in/yaml.v2"
)

const sendTimeout = 5000 // send timeout ms

// Config object
type Config struct {
	API                 string `yaml:"api"`
	Timeout             int    `yaml:"timeout"`
	Token               string `yaml:"token"`
	UseSpack            bool   `yaml:"spack"`
	UseSpackCompression bool   `yaml:"spack_compression"`
}

func LoadSenderConfig(path string) (*Config, error) {
	rawConfig, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, err
	}
	var c Config
	err = yaml.Unmarshal(rawConfig, &c)
	if err != nil {
		return nil, err
	}
	if c.API == "" {
		return nil, errors.New("API url not defined")
	}
	if c.Timeout == 0 {
		c.Timeout = sendTimeout
	}
	return &c, nil
}
