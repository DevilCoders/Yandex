package main

type RuchkaConfig struct {
	Name   string `yaml:"name"`
	Ruchka string `yaml:"ruchka"`
}

type ItsConfig struct {
	Ruchka string `yaml:"ruchka"`
	Limit  int64  `yaml:"limit"`
}

type ServiceConfig struct {
	Service string      `yaml:"service"`
	Its     []ItsConfig `yaml:"its"`
}
