package oauth_test

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/ssh"
	"golang.org/x/crypto/ssh/agent"

	"a.yandex-team.ru/library/go/yandex/oauth"
)

func TestGetTokenBySSH_files(t *testing.T) {
	type TestCase struct {
		bootstrap  func() *httptest.Server
		keyPath    string
		errMessage string
		token      string
	}
	cases := map[string]TestCase{
		"ok": {
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					data, err := ioutil.ReadAll(r.Body)
					assert.NoError(t, err)

					assert.Equal(t, "application/x-www-form-urlencoded", r.Header.Get("Content-Type"))

					expectedData := url.Values{
						"client_id":     {"client_id"},
						"client_secret": {"client_secret"},
						"grant_type":    {"ssh_key"},
						"ts":            {"1570129343"},
						"login":         {"test_login"},
						"field":         {"field_val"},
						"field1":        {"field1_val"},
						"ssh_sign": {"XFBPdfYYAOmtY7g9a3l44SYP9uqtBOeY5Gd9NLAaSaLENzVEgAOaLuO26ZRGuFsOMrk4" +
							"EoSrMrzNtO239zOm1rHaA6xNNg9zmk2QjjGXO0s1odOjaIIxMLHl99vGXAwK596mhbqE0Jf6S6g3e" +
							"Q9k5hRO1eFXjfB171Q4NJTaQSrdUpDlMg6mLQ8iz6XmBrJ0JCNi_XNttunEmNbwA-oAjGQuceHauA" +
							"wFXzWj7vYatXw3WCWfb6TQsxQ-iOMyz2vAfEjvjCJfga0E3gdfecPUVPe9UlYq88VbDQCqQ372YV0" +
							"j7sVPgGqc2YCRc1Q7TVM4-7s5Fhy_J63ofyZvQ8cWeAmzreRalrGR0T7e8D14Ah_IOD7c8Yy0bH5F" +
							"I4ZVO1KDS33I4Y-uzjuLeJ1RiFffgOWZ8g69B9q1CiDp0a-mQ7MZqJlvK2mgDTiLqfJYYZgXIC9yo" +
							"5FIlkpzGYF3Tugk6h9_EaWaKigDkAmw6Uj02YCkaelJMnA6so69H6ypfij6XiSVCrUjTWcehXDuIk" +
							"tSE1jYEocoAfVsYdiFgfBEe9zuTZMZagyhAInRAF_dS_ph_hEmQ4QH1Y5uxVp6oGTB4eUA6jHbDmE" +
							"Kxm9L51oa5yUvfztZh6qEImSP0ygBNRFwPAfocUb-L8CUqqRK1oMv5wGZcwRB7Ji1ZTwz_yIvGwQ"},
					}

					actualData, err := url.ParseQuery(string(data))
					assert.NoError(t, err)

					assert.EqualValues(t, expectedData, actualData)
					_, _ = w.Write([]byte(`{"access_token": "test_ok"}`))
				}))
			},
			keyPath:    "testdata/rsa",
			errMessage: "",
			token:      "test_ok",
		},
		"err": {
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					w.WriteHeader(http.StatusBadRequest)
					_, _ = w.Write([]byte(`{"error_description": "ooops"}`))
				}))
			},
			keyPath:    "testdata/rsa",
			errMessage: "last err: ooops",
			token:      "",
		},
	}

	tester := func(testName string, tc TestCase) {
		t.Run(testName, func(t *testing.T) {
			t.Parallel()

			srv := tc.bootstrap()
			defer srv.Close()

			keyring, err := oauth.NewSSHFileKeyring(tc.keyPath)
			require.NoError(t, err)

			token, err := oauth.GetTokenBySSH(
				context.Background(),
				"client_id",
				"client_secret",
				oauth.WithEndpoint(srv.URL),
				oauth.WithRandReader(bytes.NewReader(make([]byte, 1024*1024*10))),
				oauth.WithTime(time.Unix(1570129343, 0)),
				oauth.WithUserLogin("test_login"),
				oauth.WithAdditionalField("field", "field_val"),
				oauth.WithAdditionalField("field1", "field1_val"),
				oauth.WithSSHKeyring(keyring),
			)

			if tc.errMessage != "" {
				require.Error(t, err)
				require.Equal(t, tc.errMessage, err.Error())
				return
			}

			if tc.token != "" {
				require.NoError(t, err)
				require.Equal(t, tc.token, token)
				return
			}
		})
	}

	for name, tc := range cases {
		tester(name, tc)
	}
}

func TestGetTokenBySSH_agent(t *testing.T) {
	type SrvRsp struct {
		AccessToken string `json:"access_token"`
	}

	type TestCase struct {
		bootstrap    func(*testing.T) *httptest.Server
		keyPath      string
		certPath     string
		errMessage   string
		expectedData url.Values
	}

	bootstrap := func(t *testing.T) *httptest.Server {
		return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			data, err := ioutil.ReadAll(r.Body)
			assert.NoError(t, err)

			assert.Equal(t, "application/x-www-form-urlencoded", r.Header.Get("Content-Type"))
			err = json.NewEncoder(w).Encode(SrvRsp{
				AccessToken: string(data),
			})
			assert.NoError(t, err)
		}))
	}

	cases := map[string]TestCase{
		"rsa_key": {
			bootstrap:  bootstrap,
			keyPath:    "testdata/rsa",
			errMessage: "",
			expectedData: url.Values{
				"client_id":     {"client_id"},
				"client_secret": {"client_secret"},
				"grant_type":    {"ssh_key"},
				"ts":            {"1570129343"},
				"login":         {"test_login"},
				"field":         {"field_val"},
				"field1":        {"field1_val"},
			},
		},
		"ecdsa_key": {
			bootstrap:  bootstrap,
			keyPath:    "testdata/ecdsa",
			errMessage: "no acceptable keys available",
		},
		"ecdsa_cert": {
			bootstrap:  bootstrap,
			keyPath:    "testdata/ecdsa",
			certPath:   "testdata/ecdsa-cert.pub",
			errMessage: "",
			expectedData: url.Values{
				"client_id":     {"client_id"},
				"client_secret": {"client_secret"},
				"grant_type":    {"ssh_key"},
				"ts":            {"1570129343"},
				"login":         {"test_login"},
				"field":         {"field_val"},
				"field1":        {"field1_val"},
				"public_cert": {"AAAAKGVjZHNhLXNoYTItbmlzdHAyNTYtY2VydC12MDFAb3BlbnNzaC5jb20AAAAgx5z" +
					"7Klmsl691-qmAjXbLpcNlqfoW7_wItDnYSvzjV5QAAAAIbmlzdHAyNTYAAABBBBQ4mo9F1zNlsv92Oe" +
					"kVsWcUF5nQiDZWYWsU604zzGxdpm3c51wQq_484CIkPMeMKhcd9kAcu7D7Mt4xKcaaRIcAAAAAAAAAA" +
					"AAAAAEAAAAHc25lYWtlcgAAAAAAAAAAYeDLxAAAAABjrjkvAAAAAAAAAIIAAAAVcGVybWl0LVgxMS1m" +
					"b3J3YXJkaW5nAAAAAAAAABdwZXJtaXQtYWdlbnQtZm9yd2FyZGluZwAAAAAAAAAWcGVybWl0LXBvcnQ" +
					"tZm9yd2FyZGluZwAAAAAAAAAKcGVybWl0LXB0eQAAAAAAAAAOcGVybWl0LXVzZXItcmMAAAAAAAAAAA" +
					"AAAGgAAAATZWNkc2Etc2hhMi1uaXN0cDI1NgAAAAhuaXN0cDI1NgAAAEEE0s4mtWLHaFY6E7H_FSr3T" +
					"7ijAQRoKvQcaBhcYu7Afu950679qykpSIiz6JV3ihV_x5ryWMw7uMDaHOaO_QBh5AAAAGUAAAATZWNk" +
					"c2Etc2hhMi1uaXN0cDI1NgAAAEoAAAAhALtra-Lr_h6zoX2Sla63YMwlFY1sG0fUJylb3G0q7v-UAAA" +
					"AIQCM9Tzi3OwfBfJ5JXOSYyHBkex6wxy0FIKjOfuQGFK0tA"},
			},
		},
	}

	tester := func(testName string, tc TestCase) {
		t.Run(testName, func(t *testing.T) {
			srv := tc.bootstrap(t)
			defer srv.Close()

			sshAgent, err := newAgent(t)
			require.NoError(t, err)
			defer sshAgent.Close()

			err = sshAgent.AddKey(tc.keyPath, tc.certPath)
			require.NoError(t, err)

			_ = os.Setenv("SSH_AUTH_SOCK", sshAgent.SocketPath)
			keyring, err := oauth.NewSSHAgentKeyring()
			require.NoError(t, err)

			token, err := oauth.GetTokenBySSH(
				context.Background(),
				"client_id",
				"client_secret",
				oauth.WithEndpoint(srv.URL),
				oauth.WithRandReader(bytes.NewReader(make([]byte, 1024*1024*10))),
				oauth.WithTime(time.Unix(1570129343, 0)),
				oauth.WithUserLogin("test_login"),
				oauth.WithAdditionalField("field", "field_val"),
				oauth.WithAdditionalField("field1", "field1_val"),
				oauth.WithSSHKeyring(keyring),
			)

			if tc.errMessage != "" {
				require.Error(t, err)
				require.Equal(t, tc.errMessage, err.Error())
				return
			}
			require.NoError(t, err)

			actualData, err := url.ParseQuery(token)
			require.NoError(t, err)

			// deal with crypto/rand
			require.True(t, actualData.Has("ssh_sign"), "must have ssh_sign field")
			actualData.Del("ssh_sign")

			require.EqualValues(t, tc.expectedData, actualData)
		})
	}

	for name, tc := range cases {
		tester(name, tc)
	}
}

type SSHAgent struct {
	SocketPath string
	keyring    agent.Agent
	close      func()
}

func (s *SSHAgent) Close() {
	s.close()
	_ = os.Remove(s.SocketPath)
}

func (s *SSHAgent) AddKey(privPath, certPath string) error {
	privBytes, err := os.ReadFile(privPath)
	if err != nil {
		return fmt.Errorf("unable to read private key: %w", err)
	}

	privKey, err := ssh.ParseRawPrivateKey(privBytes)
	if err != nil {
		return fmt.Errorf("unable to parse private key: %w", err)
	}

	var cert *ssh.Certificate
	if certPath != "" {
		certBytes, err := os.ReadFile(certPath)
		if err != nil {
			return fmt.Errorf("unable to read certificate: %w", err)
		}

		pubKey, _, _, _, err := ssh.ParseAuthorizedKey(certBytes)
		if err != nil {
			return fmt.Errorf("unable to parse certificate: %w", err)
		}

		cert = pubKey.(*ssh.Certificate)
	}

	return s.keyring.Add(agent.AddedKey{
		PrivateKey:  privKey,
		Certificate: cert,
	})
}

func newAgent(t *testing.T) (*SSHAgent, error) {
	keyring := agent.NewKeyring()

	rnd, err := uuid.NewV4()
	require.NoError(t, err)

	socketPath := fmt.Sprintf("%s.sock", rnd)
	ln, err := net.Listen("unix", socketPath)
	require.NoError(t, err)

	conns := make(chan net.Conn)
	go func() {
		for {
			c, err := ln.Accept()
			switch {
			case err == nil:
				conns <- c
			case errors.Is(err, net.ErrClosed), errors.Is(err, io.ErrClosedPipe):
				return
			default:
				t.Logf("could not accept connection to agent: %v\n", err)
			}
		}
	}()

	ctx, cancel := context.WithCancel(context.Background())
	go func() {
		for {
			select {
			case <-ctx.Done():
				_ = ln.Close()
				_ = os.RemoveAll(socketPath)
				return
			case c := <-conns:
				go func() { _ = agent.ServeAgent(keyring, c) }()
			}
		}
	}()

	return &SSHAgent{
		SocketPath: socketPath,
		keyring:    keyring,
		close:      cancel,
	}, nil
}
