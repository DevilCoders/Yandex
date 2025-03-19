package accessservice

import (
	"context"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh AccessService

// AccessService defines client to access service
type AccessService interface {
	Auth(ctx context.Context, token string, permission string, resources ...Resource) (Subject, error)
	Authorize(ctx context.Context, subject cloudauth.Subject, permission string, resources ...Resource) error
}

type Resource = cloudauth.Resource

var ResourceTypeCloud = cloudauth.ResourceTypeCloud
var ResourceTypeFolder = cloudauth.ResourceTypeFolder
var ResourceTypeServiceAccount = cloudauth.ResourceTypeServiceAccount

var ResourceCloud = cloudauth.ResourceCloud
var ResourceFolder = cloudauth.ResourceFolder
var ResourceServiceAccount = cloudauth.ResourceServiceAccount

type AccountType string

const (
	AccountTypeUser      AccountType = "user_account"
	AccountTypeService   AccountType = "service_account"
	AccountTypeAnonymous AccountType = "anonymous_account"
	AccountTypeUnknown   AccountType = "unknown_account"
)

var NoIDError = xerrors.New("subject has no id")

// Subject is authorization response
type Subject struct {
	User    *UserAccount
	Service *ServiceAccount
}

func (sbj *Subject) IsEmpty() bool {
	if sbj == nil {
		return true
	}
	var empty Subject
	if *sbj == empty {
		return true
	}
	if sbj.User == nil && sbj.Service == nil {
		return true
	}
	return false
}

func (sbj *Subject) ID() (string, error) {
	if sbj.User != nil {
		return sbj.User.ID, nil
	}
	if sbj.Service != nil {
		return sbj.Service.ID, nil
	}

	return "", NoIDError
}

func (sbj *Subject) MustID() string {
	id, err := sbj.ID()
	if err != nil {
		panic(err.Error())
	}

	return id
}

// UserAccount ...
type UserAccount struct {
	ID string
}

// ServiceAccount ...
type ServiceAccount struct {
	ID       string
	FolderID string
}

func (sbj *Subject) ToGRPC() (cloudauth.Subject, error) {
	switch sbj.AccountType() {
	case AccountTypeAnonymous:
		return cloudauth.AnonymousAccount{}, nil
	case AccountTypeUser:
		return cloudauth.UserAccount{ID: sbj.User.ID}, nil
	case AccountTypeService:
		return cloudauth.ServiceAccount{ID: sbj.Service.ID, FolderID: sbj.Service.FolderID}, nil
	default:
		return nil, xerrors.Errorf("unknown subject type in response")
	}
}

func (sbj *Subject) AccountType() AccountType {
	if sbj.User == nil && sbj.Service == nil {
		return AccountTypeAnonymous
	}
	if sbj.Service == nil {
		return AccountTypeUser
	}
	if sbj.User == nil {
		return AccountTypeService
	}
	return AccountTypeUnknown
}
