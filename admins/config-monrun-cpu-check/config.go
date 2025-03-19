package main

import (
	"io/ioutil"
	"log"
	"os"

	"gopkg.in/yaml.v2"
)

const configPath = "/etc/config-monrun-cpu-check/config.yml"
const warningDefault = 70
const criticalDefault = 80
const disabledDefault = false

// Config - В конфиге хранятся различные настройки для мониторинга CPU,
// например, тресхолды warn/crit.  В будущем планируется также поддержать
// список процессов, которые буду исключаться из мониторинга,
// либо для них можно будет делать персональные трешхолды
type Config struct {
	Warning  int
	Critical int
	Disabled bool
}

// NewConfig ...
func NewConfig() *Config {
	c := new(Config)
	c.init()
	c.read()
	return c
}

func (c *Config) init() *Config {
	// Инициализируем конфиг дефолтными значениями
	c.Warning = warningDefault
	c.Critical = criticalDefault
	c.Disabled = disabledDefault
	return c
}

func (c *Config) read() {
	// Прообуем прочитать yaml-конфиг, если, конечно, он есть
	if _, err := os.Stat(configPath); os.IsNotExist(err) {
		return
	}
	yamlFile, err := ioutil.ReadFile(configPath)
	if err != nil {
		log.Fatal(err.Error())
	}
	err = yaml.Unmarshal(yamlFile, c)
	if err != nil {
		log.Fatal(err.Error())
	}
	if c.Warning == 0 || c.Critical == 0 {
		log.Fatal("Warning == 0 or Critical == 0 is not allowed")
	}
}
