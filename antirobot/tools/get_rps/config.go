package main

import (
	"bytes"
	"fmt"
	"os"
	"regexp"

	"gopkg.in/yaml.v2"
)

var envRe = regexp.MustCompile(`\{\{([0-9A-Za-z_]+)\}\}`)

type ConfigPathLoader interface {
	Load(path string) ([]byte, error)
}

type ConfigEnvLoader interface {
	Load(path string) (string, error)
}

type ConfigPathFileLoader struct{}
type ConfigEnvOSLoader struct{}

func LoadConfigFromFile(path string, container interface{}) error {
	return LoadConfig(path, &ConfigPathFileLoader{}, &ConfigEnvOSLoader{}, container)
}

func LoadConfig(path string, pathLoader ConfigPathLoader, envLoader ConfigEnvLoader, container interface{}) error {
	if dict, err := configLoad(path, pathLoader); err != nil {
		return err
	} else if data, err := yaml.Marshal(dict); err != nil {
		return err
	} else if sData, err := configSubstituteVar(data, envLoader); err != nil {
		return err
	} else {
		return yaml.Unmarshal(sData, container)
	}
}

func (l *ConfigPathFileLoader) Load(path string) ([]byte, error) {
	return os.ReadFile(path)
}

func (l *ConfigEnvOSLoader) Load(name string) (string, error) {
	val := os.Getenv(name)
	if len(val) == 0 {
		return "", fmt.Errorf("environment variable [%s] is undefined", name)
	}
	return val, nil
}

func configLoad(path string, pathLoader ConfigPathLoader) (map[string]interface{}, error) {
	res := make(map[string]interface{})

	if data, err := pathLoader.Load(path); err != nil {
		return nil, err
	} else if err = yaml.Unmarshal(data, &res); err != nil {
		return nil, err
	}

	return res, nil
}

func configSubstituteVar(data []byte, envLoader ConfigEnvLoader) ([]byte, error) {
	for _, r := range envRe.FindAll(data, -1) {
		if envVal, err := envLoader.Load(string(r[2 : len(r)-2])); err != nil {
			return nil, err
		} else {
			data = bytes.Replace(data, r, []byte(envVal), 1)
		}
	}

	return data, nil
}
