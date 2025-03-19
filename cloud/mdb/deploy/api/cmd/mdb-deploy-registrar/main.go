package main

import (
	"context"
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	_ "a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb/pg" // Load PostgreSQL backend
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	deployGroupName string
	mastersToCreate []string
)

func init() {
	pflag.StringVar(&deployGroupName, "group", "", "masters to create")
	pflag.StringSliceVar(&mastersToCreate, "masters", []string{}, "masters to create")
	flags.RegisterConfigPathFlagGlobal()
}

func main() {
	pflag.Parse()
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		fmt.Println("Error creating logger:", err)
		os.Exit(1)
	}

	ctx := context.Background()
	// Load db backend
	b, err := deploydb.Open("postgresql", l)
	if err != nil {
		l.Fatalf("Failed to open '%s' db: %s", "postgresql", err)
	}

	err = ready.Wait(ctx, b, &ready.DefaultErrorTester{Name: "deploy", L: l}, time.Second)
	if err != nil {
		l.Fatal("Failed to wait backend", log.Error(err))
	}

	var deployGroupID models.GroupID
	if group, err := b.Group(ctx, deployGroupName); err != nil {
		if !semerr.IsNotFound(err) {
			l.Fatal("Failed to check default group existence", log.Error(err))
		}
		if group, err := b.CreateGroup(ctx, deployGroupName); err != nil {
			l.Fatal("Failed to create default group", log.Error(err))
		} else {
			deployGroupID = group.ID
		}
	} else {
		l.Info("Deploy group exists")
		deployGroupID = group.ID
	}

	l.Info("Deploy group OK", log.String("group_name", deployGroupName), log.Int("group_id", int(deployGroupID)))
	l.Debugf("%d masters should exist", len(mastersToCreate))
	for index, masterToCreate := range mastersToCreate {
		_, err := b.Master(ctx, masterToCreate)
		if err == nil {
			l.Info("Master already exists", log.String("fqdn", masterToCreate))
			continue
		}

		if !semerr.IsNotFound(err) {
			l.Fatal("Failed to check master existence", log.String("fqdn", masterToCreate), log.Error(err))
		}

		l.Info("Creating master", log.String("fqdn", masterToCreate))
		if _, err := b.CreateMaster(ctx, masterToCreate, deployGroupName, true, "created during bootstrap"); err != nil {
			msg := fmt.Sprintf("Failed to create master, stopping. Masters left unprocessed: %s", strings.Join(mastersToCreate[index:], ", "))
			l.Fatal(msg, log.Error(err))
		} else {
			l.Info("Created master", log.String("fqdn", masterToCreate))
		}
	}

	l.Info("All masters created or already exist")
}
