package kfmodels

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type AccessRoleType string

const (
	AccessRoleUnspecified AccessRoleType = "unspecified"
	AccessRoleProducer    AccessRoleType = "producer"
	AccessRoleConsumer    AccessRoleType = "consumer"
	AccessRoleAdmin       AccessRoleType = "admin"
)

type User struct {
	ClusterID   string
	Name        string
	Permissions []Permission
}

type UserSpec struct {
	Name        string
	Password    secret.String
	Permissions []Permission
}

type Permission struct {
	TopicName  string
	AccessRole AccessRoleType
	Group      string
	Host       string
}

var TopicPrefixValidator = models.MustTopicPrefixValidator(models.DefaultTopicPrefixPattern, nil)

func (p Permission) Validate() error {
	if p.IsEmpty() {
		return semerr.InvalidInput("permission must be non empty")
	}
	return TopicPrefixValidator.ValidateString(p.TopicName)
}

func (p Permission) IsEmpty() bool {
	if (p == Permission{AccessRole: AccessRoleUnspecified}) {
		return true
	}
	if (p == Permission{}) {
		return true
	}
	return false
}

func MustKafkaUserNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "user name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         1,
			Max:         256,
			TooShortMsg: "user name %q is too short",
			TooLongMsg:  "user name %q is too long",
		},
		&valid.StringBlacklist{
			Blacklist: blacklist,
			Msg:       "invalid user name %q",
		},
		&valid.PrefixBlacklist{
			Blacklist: []string{"mdb_"},
			Msg:       "invalid user name %q",
		},
	)
	if err != nil {
		panic(err)
	}

	return v
}

var userNameValidator = MustKafkaUserNameValidator(models.DefaultUserNamePattern, []string{"default"})

func (us UserSpec) Validate() error {
	if err := userNameValidator.ValidateString(us.Name); err != nil {
		return err
	}

	if err := validatePassword(us.Password); err != nil {
		return err
	}

	for _, perm := range us.Permissions {
		if err := perm.Validate(); err != nil {
			return err
		}
	}

	return nil
}

func ValidateNonEmptyPassword(password secret.String) error {
	if password.Unmask() == "" {
		return semerr.InvalidInputf("user password cannot be empty")
	}
	return nil
}

func validatePassword(password secret.String) error {
	// password can be empty here during user update without changing user password
	notAllowedSymbols := "'[]"
	if strings.ContainsAny(password.Unmask(), notAllowedSymbols) {
		return semerr.InvalidInputf("password should not contain following symbols: %s", notAllowedSymbols)
	}
	return nil
}

type UpdateUserArgs struct {
	ClusterID          string
	Name               string
	Password           secret.String
	Permissions        []Permission
	PermissionsChanged bool
}

func (args UpdateUserArgs) Validate() error {
	if err := validatePassword(args.Password); err != nil {
		return err
	}
	for _, perm := range args.Permissions {
		if err := perm.Validate(); err != nil {
			return err
		}
	}
	return nil
}
