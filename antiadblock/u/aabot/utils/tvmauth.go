package main

import (
	"context"
	"log"
	"os"
	"path/filepath"

	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmauth"
)

// GetServiceTicketForConfigsApi - TVM тикет для походов в админку
func GetServiceTicketForConfigsApi() string {
	// 2000629 - PROD
	// 2000627 - TEST
	configsAPIID := GetEnvAsInt("AAB_ADMIN_TVM_CLIENT_ID", 2000627)
	configsApiTvmID := tvm.ClientID(configsAPIID)
	// https://yav.yandex-team.ru/secret/sec-01e39qc2v0z66nekdd5g79z6nq/explore/version/ver-01e39qc2w01vy1f2kbwt2sh88c
	tvmSecret := GetEnv("AABOT_TVM_SECRET", "")

	if tvmSecret == "" {
		panic("No TVM Secret")
	}

	selfID := tvm.ClientID(2019101)

	cacheDir := filepath.Join(os.TempDir(), "tvm") //"/var/tmp/cache/tvm/"

	if _, err := os.Stat(cacheDir); os.IsNotExist(err) {
		os.Mkdir(cacheDir, 0755)
	}

	tvmSettings := tvmauth.TvmAPISettings{
		SelfID: selfID,
		ServiceTicketOptions: tvmauth.NewIDsOptions(
			tvmSecret,
			[]tvm.ClientID{
				configsApiTvmID,
			}),
		DiskCacheDir: cacheDir,
	}

	tvmClient, err := tvmauth.NewAPIClient(tvmSettings, &nop.Logger{})
	if err != nil {
		panic(err)
	}
	serviceTicket, _ := tvmClient.GetServiceTicketForID(context.Background(), configsApiTvmID)
	log.Println("TIKET:", serviceTicket)
	return serviceTicket
}
