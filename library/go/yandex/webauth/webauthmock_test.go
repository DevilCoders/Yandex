package webauth

import (
	"net/http"
	"net/http/httptest"
	"strings"
)

type WebauthMock struct {
	Logins       map[string]string
	Roles        map[string][]string
	RequestCount int
}

func (m *WebauthMock) ServeHTTP(response http.ResponseWriter, request *http.Request) {
	m.RequestCount++

	if !strings.HasPrefix(request.RequestURI, "/auth_request") {
		response.WriteHeader(http.StatusNotFound)
		return
	}

	query := request.URL.Query()
	role := query.Get("idm_role")

	credentials := request.Header.Get("Authorization")

	if credentials != "" {
		credentials = strings.TrimPrefix(credentials, "OAuth ")
	} else {
		cookie, err := request.Cookie("Session_id")
		if err != nil {
			response.WriteHeader(http.StatusInternalServerError)
			_, _ = response.Write([]byte(err.Error()))
			return
		}
		credentials = cookie.Value
	}

	if credentials == "" {
		response.WriteHeader(http.StatusUnauthorized)
		return
	}

	login, ok := m.Logins[credentials]
	if !ok {
		response.WriteHeader(http.StatusUnauthorized)
		_, _ = response.Write([]byte("login not registered in mock"))
		return
	}

	response.Header().Add("X-Webauth-Login", login)
	if role == "" {
		response.WriteHeader(http.StatusOK)
	} else {
		for _, grantedRole := range m.Roles[login] {
			if role == grantedRole {
				response.WriteHeader(http.StatusOK)
				return
			}
		}

		response.WriteHeader(http.StatusForbidden)
	}
}

func NewServer(mock *WebauthMock) *httptest.Server {
	return httptest.NewUnstartedServer(mock)
}
