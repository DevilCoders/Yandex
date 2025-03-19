package steps

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type InstanceToFqdnConverterStep struct {
	meta      metadb.MetaDB
	isCompute bool
}

func (s InstanceToFqdnConverterStep) Name() string {
	return "convert instance ID to FQDN"
}

func (s InstanceToFqdnConverterStep) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) RunResult {
	var fqdn string
	instanceID := stepCtx.InstanceID()
	fqdn = stepCtx.FQDN()
	if fqdn != "" {
		ctxlog.Debug(ctx, l, "we have already known instance fqdn", log.String("FQDN", fqdn))
	} else {
		ctxlog.Debug(ctx, l, "let's find instance FQDN by instance ID")

		if s.isCompute {
			mCtx, err := s.meta.Begin(ctx, sqlutil.Alive)
			if err != nil {
				return waitWithErrAndMessage(err, "can not start metadb tx")
			}
			defer func() { _ = s.meta.Rollback(mCtx) }()

			host, err := s.meta.GetHostByVtypeID(mCtx, instanceID)
			if err != nil {
				if xerrors.Is(err, metadb.ErrDataNotFound) {
					return failWithMessageFmt("unknown instanceID %q", instanceID)
				} else {
					return waitWithErrAndMessage(err, "can't get host info from metadb")
				}
			} else {
				fqdn = host.FQDN
			}
		} else {
			res := strings.Split(instanceID, ":")
			if len(res) != 2 {
				return failWithMessageFmt("wrong instance ID format for porto installation, "+
					"it must be 'dom0_fqdn:container_fqdn', got %q", instanceID)
			}
			fqdn = res[1]
			stepCtx.SetDom0FQDN(res[0])
		}
	}
	stepCtx.SetFQDN(fqdn)
	return continueWithMessageFmt("got fqdn %q", fqdn)
}

func NewInstanceToFqdnConverterStep(db metadb.MetaDB, isCompute bool) InstanceStep {
	return &InstanceToFqdnConverterStep{
		meta:      db,
		isCompute: isCompute,
	}
}
