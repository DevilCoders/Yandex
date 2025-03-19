package steps

import (
	"bytes"
	"encoding/json"
	"io"
	"io/ioutil"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/golang/mock/gomock"

	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	s3mock "a.yandex-team.ru/cloud/mdb/internal/s3/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type S3Steps struct {
	L   log.Logger
	ctl *S3Ctl
}

func RegisterSteps(s *godog.Suite, lg log.Logger) *S3Ctl {
	s3st := &S3Steps{
		L:   lg,
		ctl: NewS3Ctl(),
	}

	s.Step(`^s3 responses sequence$`, s3st.responses)
	return s3st.ctl
}

func (s *S3Steps) responses(data *gherkin.DocString) error {
	return s.ctl.LoadResponses([]byte(data.Content))
}

type s3GetObjectResponse struct {
	Body json.RawMessage
}

type s3ListObjectsResponse struct {
	Contents       []ints3.Object
	CommonPrefixes []ints3.Prefix
}

type S3Ctl struct {
	funcs []mockFunc
}

func NewS3Ctl() *S3Ctl {
	return &S3Ctl{}
}

type mockFunc func(client *s3mock.MockClient, prev *gomock.Call) *gomock.Call

func (s3 *S3Ctl) LoadResponses(data []byte) error {
	var wrap struct {
		Responses []map[string]json.RawMessage
	}
	if err := json.Unmarshal(data, &wrap); err != nil {
		return err
	}
	var funcs []mockFunc

	for _, resp := range wrap.Responses {
		if len(resp) > 1 {
			return xerrors.Errorf("expected one key, but given: %+v", resp)
		}
		for request, response := range resp {
			switch request {
			case "ListObjects":
				var listObjects s3ListObjectsResponse
				if err := json.Unmarshal(response, &listObjects); err != nil {
					return err
				}
				funcs = append(funcs, func(s3client *s3mock.MockClient, prev *gomock.Call) *gomock.Call {
					call := s3client.EXPECT().
						ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).DoAndReturn(
						func(_, _, _ interface{}) ([]ints3.Object, []ints3.Prefix, error) {
							return listObjects.Contents, listObjects.CommonPrefixes, nil
						})
					if prev != nil {
						return call.After(prev)
					}
					return call
				})

			case "GetObject":
				var getObject s3GetObjectResponse
				if err := json.Unmarshal(response, &getObject); err != nil {
					return err
				}
				funcs = append(funcs, func(s3client *s3mock.MockClient, prev *gomock.Call) *gomock.Call {
					call := s3client.EXPECT().
						GetObject(gomock.Any(), gomock.Any(), gomock.Any()).Times(1).DoAndReturn(
						func(_, _, _ interface{}) (io.ReadCloser, error) {
							return ioutil.NopCloser(bytes.NewReader(getObject.Body)), nil
						})
					if prev != nil {
						return call.After(prev)
					}
					return call
				})
			default:
				return xerrors.Errorf("unknown request type: %s", request)
			}
		}
	}
	s3.funcs = funcs

	return nil
}

func (s3 *S3Ctl) Mock(ctl *gomock.Controller) (*s3mock.MockClient, error) {
	s3client := s3mock.NewMockClient(ctl)

	var prev *gomock.Call
	for i := range s3.funcs {
		prev = s3.funcs[i](s3client, prev)
	}

	return s3client, nil
}

func (s3 *S3Ctl) Reset() {
	s3.funcs = nil
}
