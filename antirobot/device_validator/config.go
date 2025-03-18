package main

import (
	"bufio"
	"encoding/json"
	"os"
)

type ServiceConfig struct {
	AppIDs               []string `json:"app_ids"`
	MaxTokenAgeMs        uint64   `json:"max_token_age_ms"`
	ExcludeAndroidClaims bool     `json:"exclude_android_claims"`
}

func ParseServiceConfig(path string) ([]ServiceConfig, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	decoder := json.NewDecoder(bufio.NewReader(file))
	decoder.DisallowUnknownFields()

	var serviceConfig []ServiceConfig
	err = decoder.Decode(&serviceConfig)
	if err != nil {
		return nil, err
	}

	return serviceConfig, nil
}
