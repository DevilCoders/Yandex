package metrics

import (
	"net/http"
	"testing"
)

func TestHTTPStatusMnemonics2xx(t *testing.T) {
	const expected = "2xx"

	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusOK))
	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusNoContent))
}

func TestHTTPStatusMnemonics4xx(t *testing.T) {
	const expected = "4xx"

	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusForbidden))
	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusUnauthorized))
	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusBadRequest))
}

func TestHTTPStatusMnemonics5xx(t *testing.T) {
	const expected = "5xx"

	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusInternalServerError))
	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusBadGateway))
	assertStatusCategory(t, expected, makeHTTPStatusCategory(http.StatusNotImplemented))
}

func assertStatusCategory(t *testing.T, expected, status string) {
	if status != expected {
		t.Errorf("unexpected status code %s\n", status)
	}
}
