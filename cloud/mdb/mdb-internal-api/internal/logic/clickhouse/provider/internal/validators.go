package validators

import (
	"context"
	"fmt"
	"net/http"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/library/go/core/log"
	libvalid "a.yandex-team.ru/library/go/valid"
)

type URIValidator interface {
	ValidateURI(ctx context.Context, uri string) error
}

type DummyURIValidator struct{}

func (DummyURIValidator) ValidateURI(context.Context, string) error {
	return nil
}

type httpURIValidator struct {
	client *httputil.Client
}

func NewHTTPURIValidator(settings logic.ExternalURIValidationSettings, l log.Logger) (URIValidator, error) {
	client, err := httputil.NewClient(httputil.ClientConfig{
		Name: "uri_validator",
		Transport: httputil.TransportConfig{
			TLS: settings.TLS,
		},
	}, l)

	if err != nil {
		return nil, err
	}

	client.CheckRedirect = func(req *http.Request, via []*http.Request) error {
		return semerr.InvalidInput("redirect URI is not allowed")
	}
	client.Timeout = time.Second * 10

	return httpURIValidator{client: client}, nil
}

func MustNewHTTPURIValidator(settings logic.ExternalURIValidationSettings, l log.Logger) URIValidator {
	v, err := NewHTTPURIValidator(settings, l)
	if err != nil {
		panic(fmt.Sprintf("failed to create httpURIValidator %+v", err))
	}

	return v
}

func (v httpURIValidator) ValidateURI(ctx context.Context, uri string) error {
	req, err := http.NewRequestWithContext(ctx, "GET", uri, nil)
	if err != nil {
		return err
	}

	res, err := v.client.Do(req, "validate_uri")
	if err != nil {
		return semerr.InvalidInputf("uri: %q is invalid. %v", uri, err)
	}

	return res.Body.Close()
}

const (
	DefaultExternalResourceURIMinLen = 1
	DefaultExternalResourceURIMaxLen = 512
)

type uriValidator struct {
	URIValidator
	ctx context.Context
}

func (v *uriValidator) Validate(_ *libvalid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v uriValidator) ValidateString(uri string) error {
	return v.ValidateURI(v.ctx, uri)
}

// NewExternalResourceURIValidator constructs validator for external resources
func NewExternalResourceURIValidator(ctx context.Context, validator URIValidator, name, pattern, message string) (*valid.StringComposedValidator, error) {
	msg := strings.Builder{}
	msg.WriteString(name)
	msg.WriteString(": %q is invalid. ")
	msg.WriteString(message)

	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     msg.String(),
		},
		&valid.StringLength{
			Min:         DefaultExternalResourceURIMinLen,
			Max:         DefaultExternalResourceURIMaxLen,
			TooShortMsg: fmt.Sprintf("%s is too short", name),
			TooLongMsg:  fmt.Sprintf("%s is too long", name),
			InputErrorFormatter: valid.InputErrorFormatter{
				OmitMessageFormatting: true,
			},
		},
		&uriValidator{validator, ctx},
	)
}

// MustUserPasswordValidator constructs validator for external resources. Panics on error.
func MustExternalResourceURIValidator(ctx context.Context, validator URIValidator, name, pattern, message string) *valid.StringComposedValidator {
	v, err := NewExternalResourceURIValidator(ctx, validator, name, pattern, message)
	if err != nil {
		panic(err)
	}

	return v
}
