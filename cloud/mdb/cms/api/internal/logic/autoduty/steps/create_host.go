package steps

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type NewHostsConfig struct {
	MDBDom0GroupID int `json:"group_id" yaml:"group_id"`
}

type HostInConductorStep struct {
	conductor conductor.Client
	groupID   int
}

func (s *HostInConductorStep) GetStepName() string {
	return "host created in conductor"
}

func GuessAZName(hostname string) (string, error) {
	if len(hostname) < 3 {
		return "", xerrors.New("3 chars min")
	}
	return hostname[:3], nil
}

func (s *HostInConductorStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	dom0FQDN := execCtx.GetActualRD().R.MustOneFQDN()
	az, err := GuessAZName(dom0FQDN)
	if err != nil {
		return escalateWithErrAndMsg(err, "could not guess az name")
	}
	infos, err := s.conductor.HostsInfo(ctx, []string{dom0FQDN})
	if err != nil {
		return waitWithErrAndMessage(err, "could not get info for host, will try again")

	}
	if len(infos) > 0 {
		return continueWithMessage("already exists")
	}
	dc, err := s.conductor.DCByName(ctx, az)
	if err != nil {
		return waitWithErrAndMessage(err, fmt.Sprintf("cannot get info for datacenter %q, will try again", az))
	}
	err = s.conductor.HostCreate(
		ctx,
		conductor.HostCreateRequest{
			FQDN:         dom0FQDN,
			ShortName:    strings.Split(dom0FQDN, ".")[0],
			GroupID:      s.groupID,
			DataCenterID: dc.ID,
		},
	)
	if err != nil {
		if semerr.IsAlreadyExists(err) {
			return continueWithMessage("already exists")
		}
		return waitWithErrAndMessage(err, fmt.Sprintf("cannon create host in conductor %q, will try again", dom0FQDN))
	}
	return continueWithMessage("successfully created in conductor")
}

func NewHostInConductorStep(conductor conductor.Client, conf NewHostsConfig) DecisionStep {
	return &HostInConductorStep{
		conductor: conductor,
		groupID:   conf.MDBDom0GroupID,
	}
}
