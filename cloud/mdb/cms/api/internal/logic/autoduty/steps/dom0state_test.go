package steps_test

import (
	"context"
	"encoding/json"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapimocks "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

const (
	dom0fqdn   = "some-dom.db.y.net"
	shipmentID = deploymodels.ShipmentID(42)
)

func defaultDom0State() *opmetas.Dom0StateMeta {
	return &opmetas.Dom0StateMeta{Shipments: map[string][]deploymodels.ShipmentID{dom0fqdn: {shipmentID}}}
}

func TestDom0StateStep(t *testing.T) {
	type input struct {
		state      *opmetas.Dom0StateMeta
		deployMock func(*deployapimocks.MockClient)
	}
	type expectations struct {
		forHuman string
		action   steps.AfterStepAction
		state    *opmetas.Dom0StateMeta
	}
	type tCs struct {
		name   string
		in     input
		expect expectations
	}

	for _, tc := range []tCs{
		{
			name: "first run createshipment",
			in: input{
				state: nil,
				deployMock: func(dapi *deployapimocks.MockClient) {
					dapi.EXPECT().CreateShipment(gomock.Any(), []string{dom0fqdn}, gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
						DoAndReturn(func(
							_ context.Context,
							_ []string,
							commands []deploymodels.CommandDef,
							_, _ int64,
							_ time.Duration,
						) (deploymodels.Shipment, error) {
							require.Len(t, commands, 2)
							require.Equal(t, "cmd.run", commands[0].Type)
							require.Equal(t, "grains.get", commands[1].Type)
							return deploymodels.Shipment{ID: shipmentID}, nil
						})
				},
			},
			expect: expectations{
				forHuman: "created shipments\nsuccessfully: 1",
				action:   steps.AfterStepWait,
				state:    defaultDom0State(),
			},
		},
		{
			name: "shipment is not finished",
			in: input{
				state: defaultDom0State(),
				deployMock: func(dapi *deployapimocks.MockClient) {
					dapi.EXPECT().GetShipment(gomock.Any(), deploymodels.ShipmentID(42)).Return(
						deploymodels.Shipment{
							Status: deploymodels.ShipmentStatusInProgress,
							FQDNs:  []string{dom0fqdn},
						}, nil,
					)
				},
			},
			expect: expectations{
				forHuman: "0 success:\n1 in progress:some-dom.db.y.net",
				action:   steps.AfterStepWait,
				state:    defaultDom0State(),
			},
		},
		{
			name: "timeouted shipment",
			in: input{
				state: defaultDom0State(),
				deployMock: func(dapi *deployapimocks.MockClient) {
					dapi.EXPECT().GetShipment(gomock.Any(), deploymodels.ShipmentID(42)).Return(
						deploymodels.Shipment{
							Status: deploymodels.ShipmentStatusTimeout,
							FQDNs:  []string{dom0fqdn},
						}, nil,
					)
				},
			},
			expect: expectations{
				forHuman: "This is unrecoverable till MDB-9691 and/or MDB-9634.\n" +
					"Please check why errors happened. Then click \"OK -> AT-WALLE\" button to continue.\n\n" +
					"0 failed:\n" +
					"1 timeout:some-dom.db.y.net\n" +
					"0 success:",
				action: steps.AfterStepWait,
				state:  defaultDom0State(),
			},
		},
		{
			name: "got data from shipment",
			in: input{
				state: defaultDom0State(),
				deployMock: func(dapi *deployapimocks.MockClient) {
					cmdID1 := deploymodels.CommandID(1)
					cmdID2 := deploymodels.CommandID(2)
					jobID1 := deploymodels.JobID(1)
					jobID2 := deploymodels.JobID(2)
					extJobID1 := "20220521130356460060"
					extJobID2 := "20220523102733162291"

					dapi.EXPECT().GetShipment(gomock.Any(), shipmentID).Return(
						deploymodels.Shipment{
							ID:     shipmentID,
							Status: deploymodels.ShipmentStatusDone,
							FQDNs:  []string{dom0fqdn},
						}, nil,
					)

					cmdsAttrs := deployapi.SelectCommandsAttrs{}
					cmdsAttrs.ShipmentID.Set(shipmentID.String())
					dapi.EXPECT().GetCommands(gomock.Any(), cmdsAttrs, gomock.Any()).Return([]deploymodels.Command{
						{
							ID:         cmdID1,
							ShipmentID: shipmentID,
							FQDN:       dom0fqdn,
						},
						{
							ID:         cmdID2,
							ShipmentID: shipmentID,
							FQDN:       dom0fqdn,
						},
					}, deployapi.Paging{}, nil)

					jobsAttrs := deployapi.SelectJobsAttrs{}
					jobsAttrs.ShipmentID.Set(shipmentID.String())
					dapi.EXPECT().GetJobs(gomock.Any(), jobsAttrs, gomock.Any()).Return([]deploymodels.Job{
						{
							ID:        jobID1,
							ExtID:     extJobID1,
							CommandID: cmdID1,
						},
						{
							ID:        jobID2,
							ExtID:     extJobID2,
							CommandID: cmdID2,
						},
					}, deployapi.Paging{}, nil)

					jobsResultAttrs := deployapi.SelectJobResultsAttrs{}
					jobsResultAttrs.ExtJobID.Set(extJobID1)
					jobsResultAttrs.FQDN.Set(dom0fqdn)
					dapi.EXPECT().GetJobResults(gomock.Any(), jobsResultAttrs, gomock.Any()).Return([]deploymodels.JobResult{
						{
							ExtID:  extJobID1,
							FQDN:   dom0fqdn,
							Result: json.RawMessage(`{"id": "some-dom.db.y.net", "fun": "cmd.run", "jid": "20220521130356460060", "return": "{\n  \"lldp\": {\n    \"interface\": {\n      \"eth0\": {\n        \"via\": \"LLDP\",\n        \"rid\": \"1\",\n        \"age\": \"333 days, 00:20:53\",\n        \"chassis\": {\n          \"vla1-2s58\": {\n            \"id\": {\n              \"type\": \"mac\",\n              \"value\": \"68:cc:6e:3e:f6:31\"\n            },\n            \"descr\": \"Huawei Versatile Routing Platform Software\\r\\nVRP (R) software, Version 8.191 (CE6870EI V200R019C10SPC800)\\r\\nCopyright (C) 2012-2020 Huawei Technologies Co., Ltd.\\r\\nHUAWEI CE6870-48S6CQ-EI\\r\\n\",\n            \"mgmt-ip\": [\n              \"172.24.206.67\",\n              \"2a02:6b8:c0e:130::1\"\n            ],\n            \"capability\": [\n              {\n                \"type\": \"Bridge\",\n                \"enabled\": true\n              },\n              {\n                \"type\": \"Router\",\n                \"enabled\": true\n              }\n            ]\n          }\n        },\n        \"port\": {\n          \"id\": {\n            \"type\": \"ifname\",\n            \"value\": \"10GE1/0/35\"\n          },\n          \"ttl\": \"40\",\n          \"mfs\": \"9216\",\n          \"auto-negotiation\": {\n            \"supported\": false,\n            \"enabled\": false,\n            \"current\": \"10GigBaseX - X PCS/PMA, unknown PMD.\"\n          }\n        },\n        \"vlan\": {\n          \"vlan-id\": \"333\",\n          \"pvid\": true,\n          \"value\": \"VLAN333\"\n        },\n        \"ppvid\": {\n          \"supported\": false,\n          \"enabled\": false\n        }\n      }\n    }\n  }\n}", "retcode": 0, "success": true, "fun_args": ["lldpctl -f json"]}`),
							Status: deploymodels.JobResultStatusSuccess,
						},
					}, deployapi.Paging{}, nil)

					jobsResultAttrs.ExtJobID.Set(extJobID2)
					dapi.EXPECT().GetJobResults(gomock.Any(), jobsResultAttrs, gomock.Any()).Return([]deploymodels.JobResult{
						{
							ExtID:  extJobID2,
							FQDN:   dom0fqdn,
							Result: json.RawMessage(`{"id": "some-dom.db.y.net", "fun": "grains.get", "jid": "20220523102733162291", "return": ["2a02:6b8:c0e:130:0:1589:ae37:a1b7"], "retcode": 0, "success": true, "fun_args": ["fqdn_ip6"]}`),
							Status: deploymodels.JobResultStatusSuccess,
						},
					}, deployapi.Paging{}, nil)
				},
			},
			expect: expectations{
				forHuman: "got switch \"vla1-2s58\" and port \"10GE1/0/35\", IPs: [2a02:6b8:c0e:130:0:1589:ae37:a1b7]",
				action:   steps.AfterStepContinue,
				state: &opmetas.Dom0StateMeta{
					Shipments:  map[string][]deploymodels.ShipmentID{dom0fqdn: {shipmentID}},
					SwitchPort: opmetas.SwitchPort{Switch: "vla1-2s58", Port: "10GE1/0/35"},
					IPs:        []string{"2a02:6b8:c0e:130:0:1589:ae37:a1b7"},
				},
			},
		},
		{
			name: "got switch port from state",
			in: input{
				state: &opmetas.Dom0StateMeta{
					Shipments:  map[string][]deploymodels.ShipmentID{dom0fqdn: {shipmentID}},
					SwitchPort: opmetas.SwitchPort{Switch: "vla1-2s58", Port: "10GE1/0/35"},
					IPs:        []string{"2a02:6b8:c0e:130:0:1589:ae37:a1b7"},
				},
				deployMock: func(_ *deployapimocks.MockClient) {},
			},
			expect: expectations{
				forHuman: "got switch \"vla1-2s58\" and port \"10GE1/0/35\", IPs: [2a02:6b8:c0e:130:0:1589:ae37:a1b7]",
				action:   steps.AfterStepContinue,
				state: &opmetas.Dom0StateMeta{
					Shipments:  map[string][]deploymodels.ShipmentID{dom0fqdn: {shipmentID}},
					SwitchPort: opmetas.SwitchPort{Switch: "vla1-2s58", Port: "10GE1/0/35"},
					IPs:        []string{"2a02:6b8:c0e:130:0:1589:ae37:a1b7"},
				},
			},
		},
	} {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			deploy := deployapimocks.NewMockClient(ctrl)
			deploy.EXPECT().GetMinionMaster(gomock.Any(), dom0fqdn).Return(deployapi.MinionMaster{}, nil).AnyTimes()
			tc.in.deployMock(deploy)

			step := steps.NewDom0StateStep(deploy)
			insCtx := steps.NewEmptyInstructionCtx()
			opsMetaLog := &models.OpsMetaLog{
				Dom0State: tc.in.state,
			}
			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{dom0fqdn}, Status: models.StatusInProcess},
				D: models.AutomaticDecision{OpsLog: opsMetaLog},
			})
			result := step.RunStep(ctx, &insCtx)
			require.NoError(t, result.Error)
			require.Equal(t, tc.expect.forHuman, result.ForHuman)
			require.Equal(t, tc.expect.action, result.Action)
			require.Equal(t, tc.expect.state, insCtx.GetActualRD().D.OpsLog.Dom0State)
		})
	}
}
