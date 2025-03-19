package helpers

import (
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
)

type OutMasters map[string][]OutMasterHost

type OutMasterHost struct {
	FQDN string `json:"fqdn"`
	Cid  string `json:"cid"`
}

type OutDangerHost struct {
	FQDN           string `json:"fqdn"`
	Cid            string `json:"cid"`
	SameRolesTotal int    `json:"same_roles_total"`
	SameRolesAlive int    `json:"same_roles_alive"`
	Env            string `json:"env"`
}

func NewHealthClient(env *cli.Env) client.MDBHealthClient {
	cfg := swagger.Config{Host: "health.db.yandex.net"}
	hlthcl, err := swagger.NewClientTLSFromConfig(cfg, env.L())
	if err != nil {
		env.L().Fatalf("can not create health client: %s", err)
	}
	return hlthcl
}

func NewConductorClient(env *cli.Env) conductor.Client {
	cfg := httpapi.DefaultConductorConfig()
	api, err := httpapi.New(cfg, env.L())
	if err != nil {
		env.L().Fatalf("can not create conductor client: %s", err)
	}
	return api
}

func OutputMasters(env *cli.Env, masters map[string]types.HostNeighboursInfo) {
	out := make(OutMasters)
	for fqdn, master := range masters {
		outMaster := OutMasterHost{
			FQDN: fqdn,
			Cid:  master.Cid,
		}
		for _, role := range master.Roles {
			if hosts, ok := out[role]; ok {
				out[role] = append(hosts, outMaster)
			} else {
				out[role] = []OutMasterHost{outMaster}
			}
		}
	}

	o, err := env.OutMarshaller.Marshal(out)
	if err != nil {
		env.L().Error("Failed to marshal masters", log.Any("masters", out), log.Error(err))
	}

	env.L().Info(string(o))
}

func OutputDanger(env *cli.Env, danger map[string]types.HostNeighboursInfo) {
	out := make(map[string][]OutDangerHost)
	for fqdn, host := range danger {
		for _, role := range host.Roles {
			out[role] = append(out[role], OutDangerHost{
				FQDN:           fqdn,
				Cid:            host.Cid,
				SameRolesTotal: host.SameRolesTotal,
				SameRolesAlive: host.SameRolesAlive,
				Env:            host.Env,
			})
		}
	}

	o, err := env.OutMarshaller.Marshal(out)
	if err != nil {
		env.L().Error("Failed to marshal danger CIDs", log.Any("danger", out), log.Error(err))
	}

	env.L().Info(string(o))
}
