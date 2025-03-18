package render

import (
	"bytes"
	"encoding"
	"encoding/json"
	"encoding/xml"
	"net/http"

	"github.com/golang/protobuf/proto"
	"github.com/tinylib/msgp/msgp"
	"github.com/vmihailenco/msgpack"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

var (
	ErrUnsupportedType = xerrors.NewSentinel("render: unsupported Content-Type")
	ErrEmptyAccept     = xerrors.NewSentinel("render: empty Accept header")
	ErrBadAccept       = xerrors.NewSentinel("render: bad Accept header")
)

type Renderer interface {
	Render(headers.ContentType) ([]byte, error)
}

type RendererOption func(w http.ResponseWriter)

func WithCharset(charset string) RendererOption {
	return func(w http.ResponseWriter) {
		ct := w.Header().Get(headers.ContentTypeKey)
		if ct != "" {
			w.Header().Set(headers.ContentTypeKey, ct+"; charset="+charset)
		}
	}
}

func JSON(w http.ResponseWriter, v interface{}, opts ...RendererOption) (int, error) {
	b, err := json.Marshal(v)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeApplicationJSON, b, opts...)
}

func XML(w http.ResponseWriter, v interface{}) (int, error) {
	b, err := xml.Marshal(v)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}
	// append standard XML header
	if !bytes.HasPrefix(b, []byte("<?xml")) {
		b = append([]byte(xml.Header), b...)
	}

	return writeBytes(w, headers.TypeApplicationXML, b)
}

func Protobuf(w http.ResponseWriter, v interface{}) (int, error) {
	msg, ok := v.(proto.Message)
	if !ok {
		return 0, xerrors.New("render: proto.Message expected")
	}
	b, err := proto.Marshal(msg)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeApplicationProtobuf, b)
}

func Msgpack(w http.ResponseWriter, v interface{}) (int, error) {
	switch msg := v.(type) {
	case msgp.Marshaler:
		b, err := msg.MarshalMsg(nil)
		if err != nil {
			return 0, xerrors.Errorf("render: %w", err)
		}
		return writeBytes(w, headers.TypeApplicationMsgpack, b)
	default:
		b, err := msgpack.Marshal(v)
		if err != nil {
			return 0, xerrors.New("render: msgp.Marshaler expected or msgpack.Marshal compatible")
		}
		return writeBytes(w, headers.TypeApplicationMsgpack, b)
	}
}

func OctetStream(w http.ResponseWriter, v interface{}) (int, error) {
	bm, ok := v.(encoding.BinaryMarshaler)
	if !ok {
		return 0, xerrors.New("render: binary marshaler expected")
	}

	b, err := bm.MarshalBinary()
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeApplicationOctetStream, b)
}

func marshalText(v interface{}) ([]byte, error) {
	tm, ok := v.(encoding.TextMarshaler)
	if !ok {
		return nil, xerrors.New("render: text marshaler expected")
	}

	return tm.MarshalText()
}

func TextPlain(w http.ResponseWriter, v interface{}) (int, error) {
	b, err := marshalText(v)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeTextPlain, b)
}

func TextHTML(w http.ResponseWriter, v interface{}) (int, error) {
	b, err := marshalText(v)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeTextHTML, b)
}

func TextCSV(w http.ResponseWriter, v interface{}) (int, error) {
	b, err := marshalText(v)
	if err != nil {
		return 0, xerrors.Errorf("render: %w", err)
	}

	return writeBytes(w, headers.TypeTextCSV, b)
}

//
func Write(w http.ResponseWriter, ct headers.ContentType, v interface{}) (n int, err error) {
	// render based on interface type
	switch tv := v.(type) {
	case []byte:
		return writeBytes(w, ct, tv)
	case string:
		return writeBytes(w, ct, []byte(tv))
	case Renderer:
		// call custom Render method if available
		b, err := tv.Render(ct)
		if err != nil {
			return 0, xerrors.Errorf("render: %w", err)
		}
		return writeBytes(w, ct, b)
	}

	// render based on desired Content-Type header value
	switch ct {
	case headers.TypeApplicationJSON:
		return JSON(w, v)

	case headers.TypeApplicationXML:
		return XML(w, v)

	case headers.TypeApplicationProtobuf:
		return Protobuf(w, v)

	case headers.TypeApplicationMsgpack:
		return Msgpack(w, v)

	case headers.TypeApplicationOctetStream:
		return OctetStream(w, v)

	case headers.TypeTextPlain:
		return TextPlain(w, v)

	case headers.TypeTextHTML:
		return TextHTML(w, v)

	case headers.TypeTextCSV:
		return TextCSV(w, v)

	default:
		return 0, xerrors.Errorf("%w: %s", ErrUnsupportedType, ct)
	}
}

func WriteAcceptable(w http.ResponseWriter, r *http.Request, v interface{}) (n int, err error) {
	acceptable, err := headers.ParseAccept(r.Header.Get(headers.AcceptKey))
	if err != nil {
		return 0, ErrBadAccept.Wrap(err)
	}
	if len(acceptable) == 0 {
		return 0, ErrEmptyAccept
	}

	for _, at := range acceptable {
		n, err = Write(w, at.Type, v)
		if err == nil {
			break
		}
		if !xerrors.Is(err, ErrUnsupportedType) {
			return 0, err
		}
	}

	if err != nil {
		return 0, xerrors.Errorf("none of acceptable types is supported: %s", acceptable)
	}

	return
}

func writeBytes(w http.ResponseWriter, ct headers.ContentType, b []byte, opts ...RendererOption) (n int, err error) {
	w.Header().Set(headers.ContentTypeKey, string(ct))

	for _, opt := range opts {
		opt(w)
	}

	return w.Write(b)
}
