package webauth

import (
	"context"
	"fmt"
	"log"
	"net/http/httptest"
	"net/url"
	"os"
	"testing"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/assert"
)

var (
	client *HTTPClient
	clock  clockwork.FakeClock
	mock   *WebauthMock
	server *httptest.Server
)

type Credentials struct {
	Content string
	AsToken bool
}

type TestCase struct {
	Credentials  Credentials
	Login        string
	Role         string
	RoleGranted  bool
	ExpectsError bool
	RequestCount int
}

func TestMain(m *testing.M) {
	mock = &WebauthMock{
		Logins: make(map[string]string),
		Roles:  make(map[string][]string),
	}

	server = NewServer(mock)
	server.Start()
	defer server.Close()
	fmt.Println(server.URL)

	serverURL, err := url.Parse(server.URL)
	if err != nil {
		panic(err)
	}

	clock = clockwork.NewFakeClock()

	client = NewWebauthClient(
		WithBaseURL(*serverURL),
		WithClock(clock),
	)

	os.Exit(m.Run())
}

func TestWebauthClient(t *testing.T) {
	mock.Logins["AQAD-alpha"] = "login-alpha"
	mock.Logins["AQAD-bravo"] = "login-bravo"
	mock.Logins["session:alpha"] = "login-alpha"
	mock.Logins["session:bravo"] = "login-bravo"
	mock.Roles["login-alpha"] = []string{"role-alpha"}
	mock.Roles["login-bravo"] = []string{"role-bravo"}

	t.Run("TestEmptyCredentials", testEmptyCredentials)
	t.Run("TestGetLoginByToken", testGetLoginByToken)
	t.Run("TestGetLoginBySessionID", testGetLoginBySessionID)
	t.Run("TestCheckRoleByToken", testCheckRoleByToken)
	t.Run("TestCheckRoleBySessionID", testCheckRoleBySessionID)
	t.Run("TestCaching", testCaching)

	mock.Logins["AQAD-delta"] = "login-delta"
	mock.Logins["AQAD-echo"] = "login-echo"
	mock.Roles["login-delta"] = []string{"role-zulu"}
	mock.Roles["login-echo"] = []string{}

	t.Run("TestCheckRoleBeforeGetLogin", testCheckRoleBeforeGetLogin)

	t.Run("TestCacheTTL", testCacheTTL)
}

func runGetLoginTestCases(t *testing.T, cases []*TestCase) {
	for _, c := range cases {
		var login string
		var err error

		if c.Credentials.AsToken {
			login, err = client.GetLoginByToken(context.Background(), c.Credentials.Content)
		} else {
			login, err = client.GetLoginBySessionID(context.Background(), c.Credentials.Content)
		}

		if c.ExpectsError {
			assert.Error(t, err)
		} else {
			assert.NoError(t, err)
		}

		assert.Equal(t, c.Login, login)

		if c.RequestCount != 0 {
			assert.Equal(t, c.RequestCount, mock.RequestCount)
		}
	}
}

func runCheckRoleTestCases(t *testing.T, cases []*TestCase) {
	for _, c := range cases {
		var login string
		var granted bool
		var err error

		if c.Credentials.AsToken {
			login, granted, err = client.CheckRoleByToken(context.Background(), c.Credentials.Content, c.Role)
		} else {
			login, granted, err = client.CheckRoleBySessionID(context.Background(), c.Credentials.Content, c.Role)
		}

		if c.ExpectsError {
			assert.Error(t, err)
		} else {
			assert.NoError(t, err)
		}

		assert.Equal(t, c.Login, login)
		assert.Equal(t, c.RoleGranted, granted)

		if c.RequestCount != 0 {
			assert.Equal(t, c.RequestCount, mock.RequestCount)
		}
	}
}

func testEmptyCredentials(t *testing.T) {
	// Starting with a new server.
	assert.Equal(t, 0, mock.RequestCount)

	login, err := client.GetLoginByToken(context.Background(), "")
	assert.Error(t, err)
	assert.Equal(t, "", login)

	login, err = client.GetLoginBySessionID(context.Background(), "")
	assert.Error(t, err)
	assert.Equal(t, "", login)

	// Empty credentials don't trigger an actual HTTP request.
	assert.Equal(t, 0, mock.RequestCount)
}

func testGetLoginByToken(t *testing.T) {
	runGetLoginTestCases(t, []*TestCase{
		{Credentials: Credentials{"AQAD-alpha", true}, Login: "login-alpha", RequestCount: 1},
		{Credentials: Credentials{"AQAD-bravo", true}, Login: "login-bravo", RequestCount: 2},
		{Credentials: Credentials{"AQAD-charlie", true}, ExpectsError: true, RequestCount: 3},
	})
}

func testGetLoginBySessionID(t *testing.T) {
	runGetLoginTestCases(t, []*TestCase{
		{Credentials: Credentials{"session:alpha", false}, Login: "login-alpha", RequestCount: 4},
		{Credentials: Credentials{"session:bravo", false}, Login: "login-bravo", RequestCount: 5},
		{Credentials: Credentials{"session:charlie", false}, ExpectsError: true, RequestCount: 6},
	})
}

func testCheckRoleByToken(t *testing.T) {
	runCheckRoleTestCases(t, []*TestCase{
		{Credentials: Credentials{"AQAD-alpha", true}, Role: "role-alpha", RoleGranted: true, Login: "login-alpha", RequestCount: 7},
		{Credentials: Credentials{"AQAD-alpha", true}, Role: "role-bravo", RoleGranted: false, Login: "login-alpha", RequestCount: 8},
		{Credentials: Credentials{"AQAD-bravo", true}, Role: "role-bravo", RoleGranted: true, Login: "login-bravo", RequestCount: 9},
		{Credentials: Credentials{"AQAD-bravo", true}, Role: "role-alpha", RoleGranted: false, Login: "login-bravo", RequestCount: 10},
		{Credentials: Credentials{"AQAD-charlie", true}, Role: "role-alpha", RoleGranted: false, ExpectsError: true, RequestCount: 11},
	})
}

func testCheckRoleBySessionID(t *testing.T) {
	runCheckRoleTestCases(t, []*TestCase{
		{Credentials: Credentials{"session:alpha", false}, Role: "role-alpha", RoleGranted: true, Login: "login-alpha", RequestCount: 11},
		{Credentials: Credentials{"session:alpha", false}, Role: "role-bravo", RoleGranted: false, Login: "login-alpha", RequestCount: 11},
		{Credentials: Credentials{"session:bravo", false}, Role: "role-bravo", RoleGranted: true, Login: "login-bravo", RequestCount: 11},
		{Credentials: Credentials{"session:bravo", false}, Role: "role-alpha", RoleGranted: false, Login: "login-bravo", RequestCount: 11},
		{Credentials: Credentials{"session:charlie", false}, Role: "role-alpha", RoleGranted: false, ExpectsError: true, RequestCount: 12},
	})
}

func testCaching(t *testing.T) {
	// Repeat all requests for `login-alpha` and `login-bravo` three times.
	credentials := []Credentials{
		{"AQAD-alpha", true},
		{"AQAD-bravo", true},
		{"session:bravo", false},
		{"session:bravo", false},
	}

	for i := 0; i < 3; i++ {
		for _, c := range credentials {
			if c.AsToken {
				_, _ = client.GetLoginByToken(context.Background(), c.Content)
			} else {
				_, _ = client.GetLoginBySessionID(context.Background(), c.Content)
			}

			for _, role := range []string{"role-alpha", "role-bravo"} {
				if c.AsToken {
					_, _, _ = client.CheckRoleByToken(context.Background(), c.Content, role)
				} else {
					_, _, _ = client.CheckRoleBySessionID(context.Background(), c.Content, role)
				}
			}
		}
	}

	storage := client.storage
	log.Println(storage)

	// All responses should be cached.
	assert.Equal(t, 12, mock.RequestCount)

	// Responses for `login-charlie` should not be cached.
	_, _ = client.GetLoginByToken(context.Background(), "AQAD-charlie")
	_, _ = client.GetLoginBySessionID(context.Background(), "session:charlie")
	_, _, _ = client.CheckRoleByToken(context.Background(), "AQAD-charlie", "role-alpha")
	_, _, _ = client.CheckRoleBySessionID(context.Background(), "session:charlie", "role-alpha")

	assert.Equal(t, 16, mock.RequestCount)
}

func testCheckRoleBeforeGetLogin(t *testing.T) {
	runCheckRoleTestCases(t, []*TestCase{
		{Credentials: Credentials{"AQAD-delta", true}, Role: "role-zulu", RoleGranted: true, Login: "login-delta"},
		{Credentials: Credentials{"AQAD-echo", true}, Role: "role-zulu", RoleGranted: false, Login: "login-echo"},
	})

	assert.Equal(t, 18, mock.RequestCount)

	runGetLoginTestCases(t, []*TestCase{
		{Credentials: Credentials{"AQAD-delta", true}, Login: "login-delta"},
		{Credentials: Credentials{"AQAD-echo", true}, Login: "login-echo"},
	})

	// GetLogin after CheckRole (with any role) uses cached logins.
	assert.Equal(t, 18, mock.RequestCount)
}

func testCacheTTL(t *testing.T) {
	clock.Advance(6 * time.Minute)

	_, _ = client.GetLoginByToken(context.Background(), "AQAD-alpha")
	assert.Equal(t, 19, mock.RequestCount)
}
