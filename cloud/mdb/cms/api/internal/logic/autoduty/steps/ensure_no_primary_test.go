package steps_test

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deploy_mock "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploy_models "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadb_mocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func TestPrefixFromFQDN(t *testing.T) {
	type input struct {
		fqdns []string
	}
	type output struct {
		result []string
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	tcs := []testCase{
		{
			name:   "happy path",
			input:  input{fqdns: []string{"a.db.y.net", "b.db.y.net"}},
			expect: output{result: []string{"a", "b"}},
		},
		{
			name:   "no dot",
			input:  input{fqdns: []string{"a"}},
			expect: output{result: []string{"a"}},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			require.Equal(t, tc.expect.result, steps.PrefixFromFQDNs(tc.input.fqdns))
		})
	}
}

func TestSameRegionClusterLegsByFQDN(t *testing.T) {
	type input struct {
		fqdn    string
		prepare func(db *metadb_mocks.MockMetaDB)
	}
	type output struct {
		result  []string
		withErr string
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	fqdn := "man1.db.yandex.net"
	tcs := []testCase{
		{
			name: "not found by fqdn",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)
					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{}, metadb.ErrDataNotFound)
				},
			},
			expect: output{
				withErr: fmt.Sprintf("host %s does not exist", fqdn),
			},
		}, {
			name: "2 found by shard",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					shardID := "1"
					geo := "1"
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Commit(gomock.Any()).Return(nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)

					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{
							FQDN:    fqdn,
							ShardID: optional.NewString(shardID),
						}, nil)
					db.EXPECT().GetHostsByShardID(gomock.Any(), shardID).Return(
						[]metadb.Host{
							{FQDN: fqdn, Geo: geo},
							{FQDN: "sas1.db.y.net", Geo: "anotherGeo"},
							{FQDN: "man2.db.yandex.net", Geo: geo},
						}, nil)
				},
			},
			expect: output{
				result: []string{fqdn, "man2.db.yandex.net"},
			},
		}, {
			name: "2 found by subcid",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Commit(gomock.Any()).Return(nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)

					subCid := "1"
					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{
							FQDN:         fqdn,
							SubClusterID: subCid,
						}, nil)
					db.EXPECT().GetHostsBySubcid(gomock.Any(), subCid).Return(
						[]metadb.Host{
							{FQDN: fqdn, SubClusterID: subCid},
							{FQDN: "sas1.db.y.net", Geo: "other-cid"},
							{FQDN: "man2.db.yandex.net", SubClusterID: subCid},
						}, nil)
				},
			},
			expect: output{
				result: []string{fqdn, "man2.db.yandex.net"},
			},
		}, {
			name: "only one leg",
			input: input{
				fqdn: fqdn,
				prepare: func(db *metadb_mocks.MockMetaDB) {
					db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
					db.EXPECT().Commit(gomock.Any()).Return(nil)
					db.EXPECT().Rollback(gomock.Any()).Return(nil)

					subCid := "1"
					db.EXPECT().GetHostByFQDN(gomock.Any(), fqdn).Return(
						metadb.Host{
							FQDN:         fqdn,
							SubClusterID: subCid,
						}, nil)
					db.EXPECT().GetHostsBySubcid(gomock.Any(), subCid).Return(
						[]metadb.Host{
							{FQDN: fqdn, SubClusterID: subCid},
						}, nil)
				},
			},
			expect: output{
				withErr: "is not ha",
			},
		},
	}
	for _, tc := range tcs {
		ctx := context.Background()
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			mock := metadb_mocks.NewMockMetaDB(ctrl)
			tc.input.prepare(mock)
			result, err := steps.SameRegionClusterLegsByFQDN(ctx, mock, tc.input.fqdn)
			if tc.expect.withErr == "" {
				require.NoError(t, err)
				require.Equal(t, tc.expect.result, result.FromHosts)
			} else {
				require.EqualError(t, err, tc.expect.withErr)
			}
		})
	}
}

func dummyLegsFinder(ctx context.Context, mDB metadb.MetaDB, fqdn string) (steps.WhipMasterFrom, error) {
	host, err := mDB.GetHostByFQDN(ctx, fqdn)
	if err != nil {
		panic("no error expected here")
	}
	result := steps.WhipMasterFrom{
		TargetFQDN: fqdn,
		FromHosts:  []string{host.FQDN},
	}
	return result, nil
}

func notFoundLegsFinder(_ context.Context, _ metadb.MetaDB, _ string) (steps.WhipMasterFrom, error) {
	return steps.WhipMasterFrom{}, semerr.NotFound("i am notFoundLegsFinder finder")
}

func TestEnsureNotPrimaryRun(t *testing.T) {
	type input struct {
		containesOnDom0 []string
		dom0            string
		prepare         func(containers []string, meta *metadb_mocks.MockMetaDB, deploy *deploy_mock.MockClient, dbm *mocks.MockDom0Discovery)
		metaOnInput     *opmetas.EnsureNoPrimaryMeta
		legsFinder      func(ctx context.Context, mDB metadb.MetaDB, fqdn string) (steps.WhipMasterFrom, error)
	}
	type output struct {
		action     steps.AfterStepAction
		forHuman   string
		afterSteps []steps.DecisionStep
		metaExpect *opmetas.EnsureNoPrimaryMeta
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	dom0fqdn := "some-dom.db.y.net"
	testCases := []testCase{
		{
			name: "nothing to do in first run",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					containers []string,
					meta *metadb_mocks.MockMetaDB,
					deploy *deploy_mock.MockClient,
					dbm *mocks.MockDom0Discovery,
				) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}
					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)
				},
				metaOnInput: nil,
				legsFinder:  notFoundLegsFinder,
			},
			expect: output{
				action:     steps.AfterStepContinue,
				forHuman:   "nothing to be done, 2 containers, but none to whip",
				afterSteps: []steps.DecisionStep(nil),
				metaExpect: nil,
			},
		}, {
			name: "created for 2 in single run",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					containers []string,
					meta *metadb_mocks.MockMetaDB,
					deploy *deploy_mock.MockClient,
					dbm *mocks.MockDom0Discovery,
				) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						meta.EXPECT().GetHostByFQDN(gomock.Any(), container).Return(metadb.Host{
							FQDN: container,
						}, nil)
						deploy.EXPECT().GetMinionMaster(gomock.Any(), container).Return(deployapi.MinionMaster{}, nil)
						deploy.EXPECT().CreateShipment(
							gomock.Any(),
							[]string{container},
							gomock.Any(),
							gomock.Any(),
							gomock.Any(),
							gomock.Any()).Return(deploy_models.Shipment{ID: deploy_models.ShipmentID(ind)}, nil)
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}
					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)
				},
				metaOnInput: nil,
				legsFinder:  dummyLegsFinder,
			},
			expect: output{
				action:     steps.AfterStepWait,
				forHuman:   "created shipments\nsuccessfully: 2",
				afterSteps: []steps.DecisionStep(nil),
				metaExpect: &opmetas.EnsureNoPrimaryMeta{Shipments: opmetas.ShipmentsInfo{
					"man1.db.y.net": []deploy_models.ShipmentID{0},
					"sas2.db.y.net": []deploy_models.ShipmentID{1},
				}},
			},
		}, {
			name: "first run created half shipments, second run waits till all previous are OK",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					containers []string,
					meta *metadb_mocks.MockMetaDB,
					deploy *deploy_mock.MockClient,
					dbm *mocks.MockDom0Discovery,
				) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						deploy.EXPECT().GetMinionMaster(gomock.Any(), container).Return(deployapi.MinionMaster{}, nil).AnyTimes()
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}
					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)
					deploy.EXPECT().GetShipment( // first is finished
						gomock.Any(),
						deploy_models.ShipmentID(0),
					).Return(
						deploy_models.Shipment{
							ID:     deploy_models.ShipmentID(0),
							Status: deploy_models.ShipmentStatusInProgress,
							FQDNs:  []string{"man1.db.y.net"},
						}, nil,
					)
				},
				metaOnInput: &opmetas.EnsureNoPrimaryMeta{Shipments: map[string][]deploy_models.ShipmentID{
					"man1.db.y.net": []deploy_models.ShipmentID{0},
				}},
				legsFinder: dummyLegsFinder,
			},
			expect: output{
				action:     steps.AfterStepWait,
				forHuman:   "0 success:\n1 in progress:man1.db.y.net",
				afterSteps: []steps.DecisionStep(nil),
				metaExpect: &opmetas.EnsureNoPrimaryMeta{Shipments: opmetas.ShipmentsInfo{
					"man1.db.y.net": []deploy_models.ShipmentID{0},
				}},
			},
		}, {
			name: "first run created half shipments, second run creates other half when first is success",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					containers []string,
					meta *metadb_mocks.MockMetaDB,
					deploy *deploy_mock.MockClient,
					dbm *mocks.MockDom0Discovery) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						meta.EXPECT().GetHostByFQDN(gomock.Any(), container).Return(metadb.Host{
							FQDN: container,
						}, nil).AnyTimes()
						deploy.EXPECT().GetMinionMaster(gomock.Any(), container).Return(deployapi.MinionMaster{}, nil)
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}
					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)
					deploy.EXPECT().CreateShipment(
						gomock.Any(),
						[]string{"sas2.db.y.net"},
						gomock.Any(),
						gomock.Any(),
						gomock.Any(),
						gomock.Any()).Return(deploy_models.Shipment{ID: deploy_models.ShipmentID(1)}, nil)
					deploy.EXPECT().GetShipment( // first is finished
						gomock.Any(),
						deploy_models.ShipmentID(0),
					).Return(
						deploy_models.Shipment{
							ID:     deploy_models.ShipmentID(0),
							Status: deploy_models.ShipmentStatusDone,
							FQDNs:  []string{"man1.db.y.net"},
						}, nil,
					)
				},
				metaOnInput: &opmetas.EnsureNoPrimaryMeta{Shipments: map[string][]deploy_models.ShipmentID{
					"man1.db.y.net": []deploy_models.ShipmentID{0},
				}},
				legsFinder: dummyLegsFinder,
			},
			expect: output{
				action:     steps.AfterStepWait,
				forHuman:   "created shipments\nsuccessfully: 1",
				afterSteps: []steps.DecisionStep(nil),
				metaExpect: &opmetas.EnsureNoPrimaryMeta{Shipments: opmetas.ShipmentsInfo{
					"man1.db.y.net": []deploy_models.ShipmentID{0},
					"sas2.db.y.net": []deploy_models.ShipmentID{1},
				}},
			},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			deploy := deploy_mock.NewMockClient(ctrl)
			dom0d := mocks.NewMockDom0Discovery(ctrl)
			meta := metadb_mocks.NewMockMetaDB(ctrl)
			meta.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil).AnyTimes()
			meta.EXPECT().Rollback(gomock.Any()).Return(nil).AnyTimes()
			meta.EXPECT().Commit(gomock.Any()).Return(nil).AnyTimes()
			tc.input.prepare(tc.input.containesOnDom0, meta, deploy, dom0d)

			step := steps.NewCustomEnsureNoPrimariesStep(deploy, dom0d, meta, tc.input.legsFinder)
			insCtx := steps.NewEmptyInstructionCtx()

			opsMetaLog := &models.OpsMetaLog{
				EnsureNoPrimary: tc.input.metaOnInput,
			}

			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{dom0fqdn}},
				D: models.AutomaticDecision{OpsLog: opsMetaLog},
			})
			result := step.RunStep(ctx, &insCtx)
			require.NoError(t, result.Error)
			require.Equal(t, tc.expect.forHuman, result.ForHuman)
			require.Equal(t, tc.expect.action, result.Action)
			require.Equal(t, tc.expect.afterSteps, result.AfterMeSteps)
			require.Equal(t, tc.expect.metaExpect, opsMetaLog.EnsureNoPrimary)
		})
	}
}
