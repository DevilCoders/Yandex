package masksecret

import (
	"net/http"

	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/slices"
)

var targetHeaders = []string{
	headers.AuthorizationKey,
}

// HTTPRequest masks any sensitive data for given *http.Request.
// A copy of request object will be returned if there are any data to be masked.
func HTTPRequest(r *http.Request, maskHeaders ...string) (*http.Request, error) {
	// hot path: do not mask request if no obvious sensitive parts present
	if len(r.URL.Query()) == 0 &&
		r.Header.Get(headers.AuthorizationKey) == "" &&
		r.Header.Get(headers.CookieKey) == "" &&
		len(maskHeaders) == 0 {
		return r, nil
	}

	// copy full request
	res := *r

	// copy and alter underlying request URL
	u, err := URLCopy(r.URL)
	if err != nil {
		return nil, err
	}
	res.URL = u

	// copy and alter underlying request headers
	res.Header = r.Header.Clone()
	headersToInspect := slices.DedupStrings(append(targetHeaders, maskHeaders...))
	for _, headerKey := range headersToInspect {
		headerValues := res.Header[headerKey]
		res.Header.Del(headerKey)
		for _, vv := range headerValues {
			res.Header.Add(headerKey, String(vv))
		}
	}

	// copy and alter underlying request cookies
	cookies := r.Cookies()
	res.Header.Del(headers.CookieKey)
	for _, cookie := range cookies {
		cookie.Value = String(cookie.Value)
		res.AddCookie(cookie)
	}

	return &res, nil
}
