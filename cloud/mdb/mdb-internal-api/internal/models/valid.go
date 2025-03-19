package models

import (
	"a.yandex-team.ru/cloud/mdb/internal/valid"
)

const (
	DefaultClusterNamePattern = "^[a-zA-Z0-9_-]+$"
	DefaultClusterNameMinLen  = 1
	DefaultClusterNameMaxLen  = 63
)

var ClusterNameValidator = valid.MustStringComposedValidator(
	&valid.Regexp{
		Pattern: DefaultClusterNamePattern,
		Msg:     "cluster name %q has invalid symbols",
	},
	&valid.StringLength{
		Min:         DefaultClusterNameMinLen,
		Max:         DefaultClusterNameMaxLen,
		TooShortMsg: "cluster name %q is too short",
		TooLongMsg:  "cluster name %q is too long",
	},
)

const (
	DefaultClusterDescriptionMinLen = 0
	DefaultClusterDescriptionMaxLen = 256
)

var ClusterDescriptionValidator = valid.MustStringComposedValidator(
	&valid.StringLength{
		Min:         DefaultClusterDescriptionMinLen,
		Max:         DefaultClusterDescriptionMaxLen,
		TooShortMsg: "cluster description %q is too short",
		TooLongMsg:  "cluster description %.64q is too long",
	},
)

const (
	DefaultDomainNamePattern = "^([a-z]([\\-]?[a-z0-9]+)*)([\\.][a-z]([\\-]?[a-z0-9]+)*)*$"
)

// NewDomainNameValidator constructs validator for fqdn's
func NewDomainNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "fqdn %q has invalid symbols",
		},
	)
}

// MustDomainNameValidator constructs validator for fqdn's. Panics on error.
func MustDomainNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewDomainNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultPortOfFqdnPattern = "^[1-9][0-9]{0,4}$"
)

// NewPortOfFqdnValidator constructs validator for fqdn's
func NewPortOfFqdnValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "fqdn's port %q is not valid",
		},
	)
}

// MustPortOfFqdnValidator constructs validator for fqdn's. Panics on error.
func MustPortOfFqdnValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewPortOfFqdnValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultIpv4Pattern = "^((0|([1-9][0-9]{0,2})).){3}(0|([1-9][0-9]{0,2}))$"
)

// NewIpv4Validator constructs validator for fqdn's
func NewIpv4Validator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "ipv4 address %q is not valid",
		},
	)
}

// MustIpv4Validator constructs validator for fqdn's. Panics on error.
func MustIpv4Validator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewIpv4Validator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultConnectorNamePattern = "^[-a-zA-Z0-9_\\.]+$"
	DefaultConnectorNameMinLen  = 1
	DefaultConnectorNameMaxLen  = 256
)

// NewConnectorNameValidator constructs validator for Kafka connector names
func NewConnectorNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "connector name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultConnectorNameMinLen,
			Max:         DefaultConnectorNameMaxLen,
			TooShortMsg: "connector name %q is too short, minimum is 1 symbol",
			TooLongMsg:  "connector name %q is too long, maximum is 255 symbols",
		},
		&valid.StringBlacklist{
			Blacklist: blacklist,
			Msg:       "cannot create connector with reserved name %q",
		},
	)
}

// MustConnectorNameValidator constructs validator for Kafka connector names. Panics on error.
func MustConnectorNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewConnectorNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultTopicNamePattern = "^[-a-zA-Z0-9_\\.]+$"
	DefaultTopicNameMinLen  = 1
	// Kafka has limitation 256 characters for topic name but kafka-python library has limitation 249
	// https://github.com/dpkp/kafka-python/blob/f0a57a6a20a3049dc43fbf7ad9eab9635bd2c0b0/kafka/consumer/subscription_state.py#L47
	DefaultTopicNameMaxLen = 249
)

// NewTopicNameValidator constructs validator for database names
func NewTopicNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "topic name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultTopicNameMinLen,
			Max:         DefaultTopicNameMaxLen,
			TooShortMsg: "topic name %q is too short",
			TooLongMsg:  "topic name %q is too long",
		},
		&valid.StringBlacklist{
			Blacklist: blacklist,
			Msg:       "cannot create topic with reserved name %q",
		},
	)
}

// MustTopicNameValidator constructs validator for database names. Panics on error.
func MustTopicNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewTopicNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultTopicPrefixPattern = "^[-a-zA-Z0-9_\\.]*\\*?$"
	DefaultTopicPrefixMinLen  = 1
	DefaultTopicPrefixMaxLen  = 256
)

// NewTopicPrefixValidator constructs validator for database names
func NewTopicPrefixValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "topic name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultTopicPrefixMinLen,
			Max:         DefaultTopicPrefixMaxLen,
			TooShortMsg: "topic name %q is too short",
			TooLongMsg:  "topic name %q is too long",
		},
		&valid.StringBlacklist{
			Blacklist: blacklist,
			Msg:       "invalid topic name %q",
		},
	)
}

// MustTopicPrefixValidator constructs validator for database names. Panics on error.
func MustTopicPrefixValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewTopicPrefixValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultDatabaseNamePattern = "^[a-zA-Z0-9_-]+$"
	DefaultDatabaseNameMinLen  = 1
	DefaultDatabaseNameMaxLen  = 63
)

// NewDatabaseNameValidator constructs validator for database names
func NewDatabaseNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "database name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultDatabaseNameMinLen,
			Max:         DefaultDatabaseNameMaxLen,
			TooShortMsg: "database name %q is too short",
			TooLongMsg:  "database name %q is too long",
		},
		&valid.StringBlacklist{
			Blacklist: blacklist,
			Msg:       "invalid database name %q",
		},
	)
}

// MustDatabaseNameValidator constructs validator for database names. Panics on error.
func MustDatabaseNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewDatabaseNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultUserNamePattern = "^[a-zA-Z0-9_][a-zA-Z0-9_-]*$"
	DefaultUserNameMinLen  = 1
	DefaultUserNameMaxLen  = 32
)

// NewUserNameValidator constructs validator for user names
func NewUserNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "user name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultUserNameMinLen,
			Max:         DefaultUserNameMaxLen,
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
}

// MustDatabaseNameValidator constructs validator for database names. Panics on error.
func MustUserNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewUserNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultUserPasswordPattern = ".*"
	DefaultUserPasswordMinLen  = 8
	DefaultUserPasswordMaxLen  = 128
)

// NewUserPasswordValidator constructs validator for user passwords
func NewUserPasswordValidator(pattern string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "password has invalid symbols",
			InputErrorFormatter: valid.InputErrorFormatter{
				OmitMessageFormatting: true,
			},
		},
		&valid.StringLength{
			Min:         DefaultUserPasswordMinLen,
			Max:         DefaultUserPasswordMaxLen,
			TooShortMsg: "password is too short",
			TooLongMsg:  "password is too long",
			InputErrorFormatter: valid.InputErrorFormatter{
				OmitMessageFormatting: true,
			},
		},
	)
}

// MustUserPasswordValidator constructs validator for user passwords. Panics on error.
func MustUserPasswordValidator(pattern string) *valid.StringComposedValidator {
	v, err := NewUserPasswordValidator(pattern)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultS3BucketNamePattern = "(^(([a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])\\.)*([a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])$)"
	DefaultS3BucketNameMinLen  = 3
	DefaultS3BucketNameMaxLen  = 63
)

// NewS3BucketNameValidator constructs validator for S3-bucket names.
func NewS3BucketNameValidator(pattern string, blacklist []string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "s3-bucket name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         DefaultS3BucketNameMinLen,
			Max:         DefaultS3BucketNameMaxLen,
			TooShortMsg: "s3-bucket name %q is too short",
			TooLongMsg:  "s3-bucket name %q is too long",
		},
	)
}

// MustS3BucketNameValidator constructs validator for S3 bucket names. Panics on error.
func MustS3BucketNameValidator(pattern string, blacklist []string) *valid.StringComposedValidator {
	v, err := NewS3BucketNameValidator(pattern, blacklist)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultS3PathMaxLen      = 256
	DefaultS3PathPartPattern = "^[a-zA-Z0-9_-]{3,63}$"
	DefaultS3FilenamePattern = `^[a-zA-Z0-9_\.-]+$`
	DefaultS3PathPartMinLen  = 3
	DefaultS3PathPartMaxLen  = 63
)

// NewS3PathValidator constructs validator for S3-paths.
func NewS3PathValidator(pattern string, maxlen int) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.StringLength{
			Max:        maxlen,
			TooLongMsg: "s3-path %q is too long",
		},
		&valid.PathCanonical{
			Msg: "s3-path %q should be canonical",
		},
		&valid.PathAbsolute{
			Msg: "s3-path %q should be absolute",
		},
		&valid.PathPartsValidator{
			PartValidator: &valid.Regexp{
				Pattern: pattern,
			},
			Msg: "s3-path %q has invalid symbols",
		},
	)
}

// MustS3PathValidator constructs validator for S3 bucket names. Panics on error.
func MustS3PathValidator(pattern string, maxlen int) *valid.StringComposedValidator {
	v, err := NewS3PathValidator(pattern, maxlen)
	if err != nil {
		panic(err)
	}

	return v
}

// NewS3FileNameValidator constructs validator for S3-bucket names.
func NewS3FileNameValidator(field, pattern string, minLength, maxLength int) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     field + " name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         minLength,
			Max:         maxLength,
			TooShortMsg: field + " name %q is too short",
			TooLongMsg:  field + " name %q is too long",
		},
	)
}

// MustS3FileNameValidator constructs validator for S3 bucket names. Panics on error.
func MustS3FileNameValidator(field, pattern string, minLength, maxLength int) *valid.StringComposedValidator {
	v, err := NewS3FileNameValidator(field, pattern, minLength, maxLength)
	if err != nil {
		panic(err)
	}

	return v
}

const (
	DefaultMlModelNamePattern = "^[a-zA-Z_-][a-zA-Z0-9_-]*$"
	DefaultMlModelNameMinLen  = 1
	DefaultMlModelNameMaxLen  = 63

	DefaultMLModelURIPattern = "^https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+$"
	DefaultMLModelURIMinLen  = 1
	DefaultMLModelURIMaxLen  = 512

	DefaultFormatSchemaNamePattern = "^[a-zA-Z_-][a-zA-Z0-9_-]*$"
	DefaultFormatSchemaNameMinLen  = 1
	DefaultFormatSchemaNameMaxLen  = 63

	DefaultFormatSchemaURIPattern = "^https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+$"
	DefaultFormatSchemaURIMinLen  = 1
	DefaultFormatSchemaURIMaxLen  = 512
)
