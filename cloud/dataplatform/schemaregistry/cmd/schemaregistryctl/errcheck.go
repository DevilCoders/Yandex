package main

import "a.yandex-team.ru/cloud/dataplatform/internal/logger"

func checkErr(err error) {
	if err != nil {
		logger.Log.Fatalf("error: %v", err)
	}
}
