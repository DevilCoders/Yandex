package functest

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/mock/gomock"

	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	cliApp "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/cli"
	deploymock "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IncIDGenerator struct {
	cloudIDPrefix string
	inc           int
}

func NewBackupIDGenerator(prefix string) *IncIDGenerator {
	return &IncIDGenerator{cloudIDPrefix: prefix, inc: 0}
}

func (g *IncIDGenerator) Generate() (string, error) {
	g.inc++
	return fmt.Sprintf("%s%d", g.cloudIDPrefix, g.inc), nil
}

type CliContext struct {
	HostsServiceHealth        types.ServiceStatus
	HostStatus                types.HostStatus
	ShipmentsCreationSucceeds bool
	nextShipmentID            int64
	ShipmentGetStatus         deploymodels.ShipmentStatus

	BackupIDGenerator generator.IDGenerator

	mockCtrl *gomock.Controller
}

func (cc *CliContext) Reset(lg log.Logger) {
	cc.HostsServiceHealth = types.ServiceStatusAlive
	cc.HostStatus = types.HostStatusAlive
	cc.ShipmentsCreationSucceeds = true
	cc.ShipmentGetStatus = deploymodels.ShipmentStatusUnknown
	cc.nextShipmentID = 0
	cc.BackupIDGenerator = NewBackupIDGenerator("backup_id")

	if cc.mockCtrl != nil {
		cc.mockCtrl.Finish()
	}
	cc.mockCtrl = gomock.NewController(lg)
}

func (cc *CliContext) AllShipmentsCreationFails() {
	cc.ShipmentsCreationSucceeds = false
}

func (cc *CliContext) AllShipmentsGetStatus(status string) {
	cc.ShipmentGetStatus = deploymodels.ParseShipmentStatus(status)
}

func (cc *CliContext) AllHostsServiceHealthIs(status string) error {
	var ss types.ServiceStatus
	if err := ss.UnmarshalText([]byte(status)); err != nil {
		return xerrors.Errorf("malformed service health status: %w", err)
	}
	cc.HostsServiceHealth = ss

	return nil
}

func (cc *CliContext) NewCli(ctx context.Context, s3client ints3.Client, cfg cliApp.Config, lg log.Logger) (*cliApp.Cli, error) {
	deploy := deploymock.NewMockClient(cc.mockCtrl)
	health := healthmock.NewMockMDBHealthClient(cc.mockCtrl)

	fillClusterAddr(metadbClusterName, &cfg.Metadb)
	mdb, err := mdbpg.New(cfg.Metadb, log.With(lg, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return nil, err
	}

	deploy.EXPECT().
		CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_, _, _, _, _, _ interface{}) (deploymodels.Shipment, error) {
			if !cc.ShipmentsCreationSucceeds {
				return deploymodels.Shipment{}, xerrors.Errorf("shipment creation failed")
			}
			cc.nextShipmentID++
			return deploymodels.Shipment{ID: deploymodels.ShipmentID(cc.nextShipmentID)}, nil
		})

	deploy.EXPECT().
		GetShipment(gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(
			deploymodels.Shipment{
				ID:     1,
				FQDNs:  []string{"fqdn"},
				Status: cc.ShipmentGetStatus,
			}, nil)

	health.EXPECT().
		GetHostsHealth(gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_ context.Context, fqdns []string) ([]types.HostHealth, error) {
			var ret []types.HostHealth
			for i, f := range fqdns {
				role := types.ServiceRoleMaster
				if i > 0 {
					role = types.ServiceRoleReplica
				}
				repl := types.NewServiceHealth("service_health", time.Now(), cc.HostsServiceHealth, role, types.ServiceReplicaTypeUnknown, "", 0, nil)
				ret = append(ret, types.NewHostHealthWithStatus(
					"cid",
					f,
					[]types.ServiceHealth{repl},
					cc.HostStatus,
				))
			}
			return ret, nil
		})

	cli := cliApp.NewAppFromExtDeps(cfg, mdb, deploy, health, s3client, cc.BackupIDGenerator, lg)

	awaitCtx, cancel := context.WithTimeout(ctx, 15*time.Second)
	defer cancel()
	if err = ready.Wait(awaitCtx, cli, &ready.DefaultErrorTester{Name: "worker", L: lg}, time.Second); err != nil {
		return nil, xerrors.Errorf("failed to wait backend: %w", err)
	}
	return cli, nil
}
