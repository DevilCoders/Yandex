package models

import (
	"encoding/json"
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

type Decision struct {
	ID            int64        `db:"id"`
	ReqID         int64        `db:"request_id"`
	Status        string       `db:"status"`
	Resolution    string       `db:"ad_resolution"`
	AnalysisLog   string       `db:"explanation"`
	MutationsLog  string       `db:"mutations_log"`
	AfterWalleLog string       `db:"after_walle_log"`
	CleanupLog    string       `db:"cleanup_log"`
	OpsMetaLog    pgtype.JSONB `db:"ops_metadata_log"`
	DecidedAt     time.Time    `db:"decided_at"`
}

// ToInternal casts in-db Decision model into internal one
func (d *Decision) ToInternal(log log.Logger) (models.AutomaticDecision, error) {
	s, err := models.ParseAutomaticDecision(d.Status)
	if err != nil {
		return models.AutomaticDecision{}, err
	}
	r, err := models.ParseAutomaticResolution(d.Resolution)
	if err != nil {
		return models.AutomaticDecision{}, err
	}
	opsLog, err := OpsMetaLogToInternal(d.OpsMetaLog, log)
	if err != nil {
		return models.AutomaticDecision{}, err
	}
	out := models.AutomaticDecision{
		ID:            d.ID,
		RequestID:     d.ReqID,
		Status:        s,
		AnalysisLog:   d.AnalysisLog,
		DecidedAt:     d.DecidedAt,
		MutationsLog:  d.MutationsLog,
		AfterWalleLog: d.AfterWalleLog,
		CleanupLog:    d.CleanupLog,
		Resolution:    r,
		OpsLog:        &opsLog,
	}
	return out, nil
}

type unmarshalOpMeta struct {
	StopContainers         json.RawMessage `json:"power_off,omitempty"`
	PreRestart             json.RawMessage `json:"pre_restart,omitempty"`
	Downtimes              json.RawMessage `json:"downtime,omitempty"`
	PostRestart            json.RawMessage `json:"post_restart,omitempty"`
	Dom0State              json.RawMessage `json:"dom0state,omitempty"`
	AfterWalleDom0State    json.RawMessage `json:"after_walle_dom0state,omitempty"`
	CmsInstanceWhipPrimary json.RawMessage `json:"cms_instance_whip_primary,omitempty"`
	EnsureNoPrimary        json.RawMessage `json:"ensure_no_primaries,omitempty"`
	MetadataOnCluster      json.RawMessage `json:"metadata_on_clusters,omitempty"`
	MetadataOnFQNDs        json.RawMessage `json:"metadata_on_this_dom0,omitempty"`
	LocksState             json.RawMessage `json:"locks_state,omitempty"`
	PeriodicState          json.RawMessage `json:"periodic_state,omitempty"`
}

func OpsMetaLogToDB(oml models.OpsMetaLog) (json.RawMessage, error) {
	dom0state, err := json.Marshal(oml.Dom0State)
	if err != nil {
		return nil, err
	}
	afterWalleDom0state, err := json.Marshal(oml.AfterWalleDom0State)
	if err != nil {
		return nil, err
	}
	poBytes, err := json.Marshal(oml.StopContainers)
	if err != nil {
		return nil, err
	}
	prrBytes, err := json.Marshal(oml.PreRestart)
	if err != nil {
		return nil, err
	}
	dtBytes, err := json.Marshal(oml.Downtimes)
	if err != nil {
		return nil, err
	}
	cmsInstWPBytes, err := json.Marshal(oml.CmsInstanceWhipPrimary)
	if err != nil {
		return nil, err
	}
	ensNMBytes, err := json.Marshal(oml.EnsureNoPrimary)
	if err != nil {
		return nil, err
	}
	prBytes, err := json.Marshal(oml.PostRestartContainers)
	if err != nil {
		return nil, err
	}
	mdClusterBytes, err := json.Marshal(oml.BringMetadataOnClusters)
	if err != nil {
		return nil, err
	}
	mdFQDNsBytes, err := json.Marshal(oml.BringMetadataOnFQNDs)
	if err != nil {
		return nil, err
	}
	locksState, err := json.Marshal(oml.LocksState)
	if err != nil {
		return nil, err
	}
	periodicState, err := json.Marshal(oml.PeriodicState)
	if err != nil {
		return nil, err
	}
	result, err := json.Marshal(&unmarshalOpMeta{
		PreRestart:             prrBytes,
		StopContainers:         poBytes,
		Downtimes:              dtBytes,
		PostRestart:            prBytes,
		Dom0State:              dom0state,
		AfterWalleDom0State:    afterWalleDom0state,
		CmsInstanceWhipPrimary: cmsInstWPBytes,
		EnsureNoPrimary:        ensNMBytes,
		MetadataOnCluster:      mdClusterBytes,
		MetadataOnFQNDs:        mdFQDNsBytes,
		LocksState:             locksState,
		PeriodicState:          periodicState,
	})
	return result, err
}

func OpsMetaLogToInternal(dbOML pgtype.JSONB, log log.Logger) (models.OpsMetaLog, error) {
	result := models.NewOpsMetaLog()
	tmpOpMeta := &unmarshalOpMeta{}
	err := json.Unmarshal(dbOML.Bytes, tmpOpMeta)
	if err != nil {
		return result, err
	}
	if err = json.Unmarshal(tmpOpMeta.Downtimes, &result.Downtimes); err != nil {
		log.Infof("could not unmarshal Downtimes: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.PreRestart, &result.PreRestart); err != nil {
		log.Infof("could not unmarshal PreRestart: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.StopContainers, &result.StopContainers); err != nil {
		log.Infof("could not unmarshal StopContainers: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.PostRestart, &result.PostRestartContainers); err != nil {
		log.Infof("could not unmarshal PostRestart: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.Dom0State, &result.Dom0State); err != nil {
		log.Infof("could not unmarshal Dom0State: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.AfterWalleDom0State, &result.AfterWalleDom0State); err != nil {
		log.Infof("could not unmarshal AfterWalleDom0State: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.CmsInstanceWhipPrimary, &result.CmsInstanceWhipPrimary); err != nil {
		log.Infof("could not unmarshal CmsInstanceWhipPrimary: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.EnsureNoPrimary, &result.EnsureNoPrimary); err != nil {
		log.Infof("could not unmarshal EnsureNoPrimary: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.MetadataOnCluster, &result.BringMetadataOnClusters); err != nil {
		log.Infof("could not unmarshal MetadataOnCluster: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.MetadataOnFQNDs, &result.BringMetadataOnFQNDs); err != nil {
		log.Infof("could not unmarshal MetadataOnFQNDs: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.LocksState, &result.LocksState); err != nil {
		log.Infof("could not unmarshal LocksState: %s, using default value", err)
	}
	if err = json.Unmarshal(tmpOpMeta.PeriodicState, &result.PeriodicState); err != nil {
		log.Infof("could not unmarshal PeriodicState: %s, using default value", err)
	}
	return result, nil
}
