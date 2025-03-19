package main

import (
	"context"
	"os"
)

type lbInstallation struct {
	Host       string
	Port       int
	DB         string
	DisableTLS bool
}

var (
	lbInstallations = map[string]lbInstallation{
		"yc_preprod": {
			Host: "lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net",
			Port: 2135,
			DB:   "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
		},
		"lbk": {
			Host:       "vla.logbroker.yandex.net",
			Port:       2135,
			DB:         "/Root",
			DisableTLS: true,
		},
		"lbkx": {
			Host:       "lbkx.logbroker.yandex.net",
			Port:       2135,
			DB:         "/Root",
			DisableTLS: true,
		},
	}

	token string = os.Getenv("IAM_TOKEN")
)

type tokenCreds string

func (t tokenCreds) Token(context.Context) (string, error) {
	return string(t), nil
}
