package utils

import (
	"bytes"
	"context"
	"crypto/md5"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"runtime"
	"strconv"
	"time"

	"github.com/BurntSushi/toml"
	"github.com/google/uuid"
	yaml "gopkg.in/yaml.v2"
)

// https://blog.sgmansfield.com/2015/12/goroutine-ids/
func GetGID() uint64 {
	b := make([]byte, 64)
	b = b[:runtime.Stack(b, false)]
	b = bytes.TrimPrefix(b, []byte("goroutine "))
	b = b[:bytes.IndexByte(b, ' ')]
	n, _ := strconv.ParseUint(string(b), 10, 64)
	return n
}

func GetGPid() string {
	return fmt.Sprintf("[%d]:[%d]", os.Getpid(), GetGID())
}

// Checkings from LXD
func StringInSlice(key string, list []string) bool {
	for _, entry := range list {
		if entry == key {
			return true
		}
	}
	return false
}

// ConvMarshal converts YSON(+RAW)/TOML/YAML
func ConvMarshal(t interface{}, mode string) (string, error) {
	var text []byte
	var err error

	if mode == "TOML" {
		var buffer bytes.Buffer
		var encoder = toml.NewEncoder(&buffer)
		if err = encoder.Encode(t); err != nil {
			return "", nil
		}
		text = buffer.Bytes()
	}

	if mode == "YAML" {
		text, err = yaml.Marshal(t)
	}
	if mode == "JSON" {
		text, err = json.MarshalIndent(t, "", "\t")
	}
	if mode == "JSON+RAW" {
		text, err = json.Marshal(t)
	}
	if err != nil {
		return "", err
	}
	return string(text), nil
}

func IntInSlice(key int, list []int) bool {
	for _, entry := range list {
		if entry == key {
			return true
		}
	}
	return false
}

// Http request and response handling for GET/POST requets with authorization token
func HttpRequest(method string, url string, token string, content []byte,
	transport *http.Transport, useragent string) ([]byte, error, int) {
	var err error
	var req *http.Request
	var res *http.Response
	var body []byte

	Client := http.Client{
		Timeout: time.Second * 600,
	}

	if transport != nil {
		Client.Transport = transport
	}

	req, err = http.NewRequest(method, url, bytes.NewBuffer(content))
	if err != nil {
		return nil, err, 0
	}

	u := "hx/3.0"
	if len(useragent) > 0 {
		u = useragent
	}
	req.Header.Set("User-Agent", u)
	switch token {
	case "XML":
		if method != http.MethodGet {
			req.Header.Set("Content-Type", "application/xml")
		}
		req.Header.Set("Accept", "application/xml")
	default:
		if method != http.MethodGet {
			req.Header.Set("Content-Type", "application/json")
		}
		req.Header.Set("Accept", "application/json")
	}

	if len(token) > 0 {
		req.Header.Set("Authorization", fmt.Sprintf("OAuth %s", token))
	}

	if res, err = Client.Do(req); err != nil {
		return nil, err, 0
	}
	defer res.Body.Close()

	if body, err = ioutil.ReadAll(res.Body); err != nil {
		return nil, err, res.StatusCode
	}

	if !IntInSlice(res.StatusCode, []int{200, 201, 202}) {
		return body, errors.New(fmt.Sprintf("Request failed, http code:'%d', error:'%s'",
			res.StatusCode, http.StatusText(res.StatusCode))), res.StatusCode
	}

	return body, nil, res.StatusCode
}

// Http Unix Request
func HttpUnixRequest(method string, url string, socket string, content []byte) ([]byte, error, int) {

	transport := &http.Transport{
		DialContext: func(_ context.Context, _, _ string) (net.Conn, error) {
			return net.Dial("unix", socket)
		},
	}

	return HttpRequest(method, fmt.Sprintf("http://unix/%s", url),
		"", content, transport, "")
}

// Getting UUID
func Id() (string, error) {
	u := uuid.New()
	return u.String(), nil
}

// WriteCloserBuffer decorates a byte buffer with the Closer interface.
type WriteCloserBuffer struct {
	*bytes.Buffer
}

// NewWriteCloserBuffer creates a write closer buffer
func NewWriteCloserBuffer() *WriteCloserBuffer {
	return &WriteCloserBuffer{&bytes.Buffer{}}
}

// Close does nothing
func (m WriteCloserBuffer) Close() error {
	fmt.Println("closed closer")
	return nil
}

func Exists(name string) bool {
	if _, err := os.Stat(name); err != nil {
		if os.IsNotExist(err) {
			return false
		}
	}
	return true
}

func Md5(s string) string {
	h := md5.New()
	io.WriteString(h, s)
	return fmt.Sprintf("%x", h.Sum(nil))
}
