package main

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var byDom0, byClusterName string

func init() {
	pflag.StringVarP(&byDom0, "dom0", "d", "", "show containers by dom0")
	pflag.StringVarP(&byClusterName, "cluster-name", "c", "", "show containers by cluster name")
}

func main() {
	pflag.Parse()

	L, err := zap.New(zap.CLIConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	token, ok := os.LookupEnv("DBM_TOKEN")
	if !ok {
		L.Fatalf("set DBM_TOKEN variable")
	}
	cfg := restapi.DefaultConfig()

	cfg.Token = secret.NewString(token)

	client, err := restapi.New(cfg, L)
	if err != nil {
		L.Fatal("client initialization failed", log.Error(err))
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()

	printContainers := func(containers []dbm.Container, err error) {
		if err != nil {
			L.Fatal("failed with", log.Error(err))
		}
		for _, c := range containers {
			fmt.Printf("%+v\n", c)
		}
	}
	if len(byClusterName) > 0 {
		fmt.Printf("containers by cluster name: %q\n", byClusterName)
		printContainers(client.ClusterContainers(ctx, byClusterName))
	}
	if len(byDom0) > 0 {
		fmt.Printf("containers by cluster name: %q\n", byDom0)
		printContainers(client.Dom0Containers(ctx, byDom0))
	}
}
