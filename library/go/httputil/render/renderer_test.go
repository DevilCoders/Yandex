//go:build !nolint
// +build !nolint

package render_test

import (
	"bytes"
	"encoding/csv"
	"encoding/xml"
	"html/template"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"
	"github.com/tinylib/msgp/msgp"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/httputil/render"
	"a.yandex-team.ru/library/go/httputil/render/testproto"
)

type testRenderer struct {
	body string
}

func (r testRenderer) Render(_ headers.ContentType) ([]byte, error) {
	return []byte("prerender-" + r.body), nil
}

type testXML struct {
	XMLName xml.Name
	Val     int `xml:"value"`
}

type testXML42 struct {
	XMLName xml.Name
	Val     int `xml:"value"`
}

type testMsgp struct {
	Val int `msg:"value"`
}

type testMsgpWithooutTags struct {
	Val int
}

func (r testMsgp) MarshalMsg(o []byte) (b []byte, err error) {
	b = msgp.AppendMapHeader(o, 1)
	b = msgp.AppendString(b, "value")
	b = msgp.AppendInt(b, r.Val)
	return
}

type testMultiformat struct {
	Val string `msg:"value" json:"value"`
}

func (r testMultiformat) MarshalJSON() (b []byte, err error) {
	return []byte(`{"value":"` + r.Val + `"}`), nil
}

func (r testMultiformat) MarshalMsg(o []byte) (b []byte, err error) {
	b = msgp.AppendMapHeader(o, 1)
	b = msgp.AppendString(b, "value")
	b = msgp.AppendString(b, r.Val)
	return
}

type testHTML struct {
	Val string
}

func (r testHTML) MarshalText() ([]byte, error) {
	tpl := `<html><body><h1>{{.Val}}</h1></body></html>`
	t, err := template.New("test").Parse(tpl)
	if err != nil {
		return nil, err
	}

	buf := bytes.NewBuffer(nil)
	err = t.Execute(buf, r)
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

type testCSV struct {
	Data [][]string
}

func (r testCSV) MarshalText() ([]byte, error) {
	buf := bytes.NewBuffer(nil)
	err := csv.NewWriter(buf).WriteAll(r.Data)
	if err != nil {
		return nil, err
	}
	return buf.Bytes(), nil
}

var (
	testBinaryUUID    = uuid.Must(uuid.NewV4())
	testPlaintextUUID = uuid.Must(uuid.NewV4())
)

func TestWrite(t *testing.T) {
	testCases := []struct {
		name        string
		contentType headers.ContentType
		value       interface{}

		expectedContentType string
		expectedBody        []byte
		expectedWritten     int
		expectedErr         error
	}{
		{
			"bytes",
			headers.TypeApplicationOctetStream,
			[]byte{0x4a, 0x4f, 0x50, 0x41}, // eternity

			string(headers.TypeApplicationOctetStream),
			[]byte{0x4a, 0x4f, 0x50, 0x41},
			4,
			nil,
		},
		{
			"string",
			headers.TypeTextPlain,
			"eternity",

			string(headers.TypeTextPlain),
			[]byte("eternity"),
			8,
			nil,
		},
		{
			"custom_renderer",
			headers.TypeApplicationOctetStream,
			testRenderer{body: "test"},

			string(headers.TypeApplicationOctetStream),
			[]byte("prerender-test"),
			14,
			nil,
		},
		{
			"json",
			headers.TypeApplicationJSON,
			map[string]interface{}{"test": 42},

			string(headers.TypeApplicationJSON),
			[]byte(`{"test":42}`),
			11,
			nil,
		},
		{
			"protobuf",
			headers.TypeApplicationProtobuf,
			&testproto.Test{Value: 42},

			string(headers.TypeApplicationProtobuf),
			[]byte{0x8, 0x2a},
			2,
			nil,
		},
		{
			"messagepack",
			headers.TypeApplicationMsgpack,
			testMsgp{Val: 42},

			string(headers.TypeApplicationMsgpack),
			[]byte{0x81, 0xa5, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x2a},
			8,
			nil,
		},
		{
			"messagepack - struct without msgp tags",
			headers.TypeApplicationMsgpack,
			testMsgpWithooutTags{Val: 42},

			string(headers.TypeApplicationMsgpack),
			[]byte{0x81, 0xa3, 0x56, 0x61, 0x6c, 0xd3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a},
			14,
			nil,
		},
		{
			"binary",
			headers.TypeApplicationOctetStream,
			testBinaryUUID,

			string(headers.TypeApplicationOctetStream),
			testBinaryUUID.Bytes(),
			16,
			nil,
		},
		{
			"text_plain",
			headers.TypeTextPlain,
			testPlaintextUUID,

			string(headers.TypeTextPlain),
			[]byte(testPlaintextUUID.String()),
			36,
			nil,
		},
		{
			"html",
			headers.TypeTextHTML,
			testHTML{Val: "The Ultimate Answer..."},

			string(headers.TypeTextHTML),
			[]byte(`<html><body><h1>The Ultimate Answer...</h1></body></html>`),
			57,
			nil,
		},
		{
			"csv",
			headers.TypeTextCSV,
			testCSV{Data: [][]string{
				{"So", "long"},
				{"and", "thanks"},
				{"for", "all"},
				{"the", "fish"},
			}},

			string(headers.TypeTextCSV),
			[]byte("So,long\nand,thanks\nfor,all\nthe,fish\n"),
			36,
			nil,
		},
		{
			"unsupported_type",
			headers.ContentType("custom/unsupported"),
			uuid.Must(uuid.NewV4()),

			"",
			nil,
			0,
			xerrors.Errorf("%w: custom/unsupported", render.ErrUnsupportedType),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			rec := httptest.NewRecorder()
			written, err := render.Write(rec, tc.contentType, tc.value)

			if tc.expectedErr != nil {
				assert.EqualError(t, err, tc.expectedErr.Error())
			} else {
				assert.NoError(t, err)
			}

			assert.Equal(t, tc.expectedWritten, written)
			assert.Equal(t, tc.expectedContentType, rec.Header().Get(headers.ContentTypeKey))
			assert.Equal(t, tc.expectedBody, rec.Body.Bytes())
		})
	}
}

func TestWriteAcceptable(t *testing.T) {
	ctReq := func(ac string) *http.Request {
		r, _ := http.NewRequest("GET", "/", nil)
		r.Header.Set(headers.AcceptKey, ac)
		return r
	}

	testCases := []struct {
		name  string
		req   *http.Request
		value interface{}

		expectedContentType string
		expectedBody        []byte
		expectedWritten     int
		expectedErr         error
	}{
		{
			"empty_header",
			ctReq(""),
			[]byte{0x4a, 0x4f, 0x50, 0x41}, // eternity

			"",
			nil,
			0,
			render.ErrEmptyAccept,
		},
		{
			"bytes",
			ctReq(string(headers.TypeApplicationOctetStream)),
			[]byte{0x4a, 0x4f, 0x50, 0x41}, // eternity

			string(headers.TypeApplicationOctetStream),
			[]byte{0x4a, 0x4f, 0x50, 0x41},
			4,
			nil,
		},
		{
			"string",
			ctReq(string(headers.TypeApplicationOctetStream)),
			"eternity",

			string(headers.TypeApplicationOctetStream),
			[]byte("eternity"),
			8,
			nil,
		},
		{
			"custom_renderer",
			ctReq(string(headers.TypeApplicationOctetStream)),
			testRenderer{body: "test"},

			string(headers.TypeApplicationOctetStream),
			[]byte("prerender-test"),
			14,
			nil,
		},
		{
			"json",
			ctReq(string(headers.TypeApplicationJSON)),
			map[string]interface{}{"test": 42},

			string(headers.TypeApplicationJSON),
			[]byte(`{"test":42}`),
			11,
			nil,
		},
		{
			"protobuf",
			ctReq(string(headers.TypeApplicationProtobuf)),
			&testproto.Test{Value: 42},

			string(headers.TypeApplicationProtobuf),
			[]byte{0x8, 0x2a},
			2,
			nil,
		},
		{
			"messagepack",
			ctReq(string(headers.TypeApplicationMsgpack)),
			testMsgp{Val: 42},

			string(headers.TypeApplicationMsgpack),
			[]byte{0x81, 0xa5, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x2a},
			8,
			nil,
		},
		{
			"binary",
			ctReq(string(headers.TypeApplicationOctetStream)),
			testBinaryUUID,

			string(headers.TypeApplicationOctetStream),
			testBinaryUUID.Bytes(),
			16,
			nil,
		},
		{
			"text_plain",
			ctReq(string(headers.TypeTextPlain)),
			testPlaintextUUID,

			string(headers.TypeTextPlain),
			[]byte(testPlaintextUUID.String()),
			36,
			nil,
		},
		{
			"html",
			ctReq(string(headers.TypeTextHTML)),
			testHTML{Val: "The Ultimate Answer..."},

			string(headers.TypeTextHTML),
			[]byte(`<html><body><h1>The Ultimate Answer...</h1></body></html>`),
			57,
			nil,
		},
		{
			"csv",
			ctReq(string(headers.TypeTextCSV)),
			testCSV{Data: [][]string{
				{"So", "long"},
				{"and", "thanks"},
				{"for", "all"},
				{"the", "fish"},
			}},

			string(headers.TypeTextCSV),
			[]byte("So,long\nand,thanks\nfor,all\nthe,fish\n"),
			36,
			nil,
		},
		{
			"multiformat_json",
			ctReq("application/json;q=0.8, application/msgpack;q=0.5"),
			testMultiformat{Val: "Hello"},

			string(headers.TypeApplicationJSON),
			[]byte(`{"value":"Hello"}`),
			17,
			nil,
		},
		{
			"multiformat_msgp",
			ctReq("application/json;q=0.5, application/msgpack;q=0.8"),
			testMultiformat{Val: "Hello"},

			string(headers.TypeApplicationMsgpack),
			[]byte{0x81, 0xa5, 0x76, 0x61, 0x6c, 0x75, 0x65, 0xa5, 0x48, 0x65, 0x6c, 0x6c, 0x6f},
			13,
			nil,
		},
		{
			"last_acceptable_type",
			ctReq("some/content_type, other/content_type;q=0.8, application/json;q=0.1"),
			map[string]interface{}{"value": 42},

			string(headers.TypeApplicationJSON),
			[]byte(`{"value":42}`),
			12,
			nil,
		},
		{
			"no_acceptable_type",
			ctReq("some/content_type, other/content_type;q=0.7"),
			testPlaintextUUID,

			"",
			nil,
			0,
			xerrors.New("none of acceptable types is supported: some/content_type, other/content_type;q=0.7"),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			rec := httptest.NewRecorder()
			written, err := render.WriteAcceptable(rec, tc.req, tc.value)

			if tc.expectedErr != nil {
				assert.EqualError(t, err, tc.expectedErr.Error())
			} else {
				assert.NoError(t, err)
			}

			assert.Equal(t, tc.expectedWritten, written)
			assert.Equal(t, tc.expectedContentType, rec.Header().Get(headers.ContentTypeKey))
			assert.Equal(t, tc.expectedBody, rec.Body.Bytes())
		})
	}
}
