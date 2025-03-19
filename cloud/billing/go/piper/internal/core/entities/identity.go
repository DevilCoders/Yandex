package entities

import "time"

type BillingAccount struct {
	AccountID       string
	MasterAccountID string
}

type Folder struct {
	FolderID string
	CloudID  string
}

type AbcFolder struct {
	AbcID       int64
	AbcFolderID string
	CloudID     string
}

type CloudAtTime struct {
	CloudID string
	At      time.Time
}

type CloudBinding struct {
	CloudID        string
	BillingAccount string
	BindingTimes
}

type ResourceAtTime struct {
	ResourceBindingKey
	At time.Time
}

type ResourceBinding struct {
	ResourceBindingKey
	BindingTimes
	BillingAccount string
}

type ResourceBindingKey struct {
	ResourceID  string
	BindingType ResourceBindingType
}

type BindingTimes struct {
	EffectiveFrom time.Time
	EffectiveTo   time.Time // Effective time from next record of same CloudID
}

func (bt BindingTimes) Belongs(t time.Time) bool {
	return !t.Before(bt.EffectiveFrom) &&
		(bt.EffectiveTo.IsZero() || bt.EffectiveTo.After(t))
}
