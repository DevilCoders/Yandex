package models

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OpsMetaLog struct {
	Dom0State               *opmetas.Dom0StateMeta
	AfterWalleDom0State     *opmetas.Dom0StateMeta
	PreRestart              PreRestarts
	StopContainers          StopContainers
	PostRestartContainers   PostRestarts
	Downtimes               Downtimes
	EnsureNoPrimary         *opmetas.EnsureNoPrimaryMeta
	CmsInstanceWhipPrimary  *opmetas.CmsInstanceWhipPrimaryMeta
	BringMetadataOnClusters *opmetas.BringMetadataMeta
	BringMetadataOnFQNDs    *opmetas.BringMetadataMeta
	LocksState              *opmetas.LocksStateMeta
	PeriodicState           *opmetas.PeriodicStateMeta
}

func NewOpsMetaLog() OpsMetaLog {
	return OpsMetaLog{}
}

func (l *OpsMetaLog) Add(o opmetas.OpMeta) error {
	switch o := o.(type) {
	case *opmetas.PreRestartStepMeta:
		l.PreRestart = append(l.PreRestart, *o)
	case *opmetas.SetDowntimesStepMeta:
		l.Downtimes = append(l.Downtimes, *o)
	case *opmetas.PostRestartMeta:
		l.PostRestartContainers = append(l.PostRestartContainers, *o)
	case *opmetas.StopContainersMeta:
		l.StopContainers = append(l.StopContainers, *o)
	case *opmetas.Dom0StateMeta:
		l.Dom0State = o
	case *opmetas.BringMetadataMeta:
		l.BringMetadataOnFQNDs = o
	case *opmetas.LocksStateMeta:
		l.LocksState = o
	case *opmetas.PeriodicStateMeta:
		l.PeriodicState = o
	default:
		return opmetas.UnknownOpTypeErr.Wrap(xerrors.Errorf("%T", o))
	}
	return nil
}

type Downtimes []opmetas.SetDowntimesStepMeta

type PostRestarts []opmetas.PostRestartMeta

type PreRestarts []opmetas.PreRestartStepMeta

type StopContainers []opmetas.StopContainersMeta

func (metas *Downtimes) Latest() *opmetas.SetDowntimesStepMeta {
	cnt := *metas
	length := len(cnt)
	if length == 0 {
		return nil
	}
	return &cnt[length-1]
}

func (metas *PostRestarts) Latest() *opmetas.PostRestartMeta {
	cnt := *metas
	length := len(cnt)
	if length == 0 {
		return nil
	}
	return &cnt[length-1]
}

func (metas *StopContainers) Latest() *opmetas.StopContainersMeta {
	cnt := *metas
	length := len(cnt)
	if length == 0 {
		return nil
	}
	return &cnt[length-1]
}

func (metas *PreRestarts) Latest() *opmetas.PreRestartStepMeta {
	cnt := *metas
	length := len(cnt)
	if length == 0 {
		return nil
	}
	return &cnt[length-1]
}
