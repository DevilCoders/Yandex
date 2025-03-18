package httpyav

import (
	"crypto/rsa"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"

	"golang.org/x/crypto/ssh"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var timeNow = time.Now

// signs request using given RSA private key
func rsaSignRequest(req *http.Request, login string, key *rsa.PrivateKey) error {
	if key == nil {
		return nil
	}

	ts := time.Now()
	body, err := req.GetBody()
	if err != nil {
		return fmt.Errorf("request GetBody error: %w", err)
	}
	payload, err := json.Marshal(body)
	if err != nil {
		return xerrors.Errorf("cannot marshal body before signing request: %w", err)
	}

	serialized := serializeRequest(req.Method, req.URL.Path, payload, ts, login)
	signature, err := computeRSASignature(serialized, key)
	if err != nil {
		return xerrors.Errorf("cannot sign request with RSA key: %w", err)
	}

	req.Header.Set("X-Ya-Rsa-Signature", signature)
	req.Header.Set("X-Ya-Rsa-Login", login)
	req.Header.Set("X-Ya-Rsa-Timestamp", strconv.FormatInt(ts.Unix(), 10))

	return nil
}

// agentRsaSignRequest signs request using ssh agent
func agentRsaSignRequest(req *http.Request, login string, signer *ssh.Signer) error {
	ts := timeNow()

	serialized, err := serializeRequestV3(*req, ts, login)
	if err != nil {
		return xerrors.Errorf("signing error: %w", err)
	}
	signature, err := computeRSASignatureByAgent(serialized, *signer)
	if err != nil {
		return xerrors.Errorf("signing error: %w", err)
	}

	req.Header.Set("X-Ya-Rsa-Signature", signature)
	req.Header.Set("X-Ya-Rsa-Login", login)
	req.Header.Set("X-Ya-Rsa-Timestamp", strconv.FormatInt(ts.Unix(), 10))
	return nil
}

func serializeRequest(method, path string, payload []byte, ts time.Time, login string) string {
	return fmt.Sprintf("%s\n%s\n%s\n%d\n%s\n", strings.ToUpper(method), path, payload, ts.Unix(), login)
}

// serializeRequestV3 serializes request
// see: https://a.yandex-team.ru/arc/trunk/arcadia/passport/backend/vault/api/views/base_view.py?rev=r9165057#L320
func serializeRequestV3(req http.Request, ts time.Time, login string) (string, error) {
	var payload []byte
	var err error
	// see: https://a.yandex-team.ru/arc/trunk/arcadia/library/python/vault_client/vault_client/client.py?rev=r9165074#L281
	// otherwise nil will be encoded as 'null' not ''
	body, err := req.GetBody()
	if err != nil {
		return "", fmt.Errorf("request GetBody error: %w", err)
	}
	if body != nil {
		payload, err = json.Marshal(body)
		if err != nil {
			return "", xerrors.Errorf("cannot marshal body before signing request: %w", err)
		}
	}
	// see: https://a.yandex-team.ru/arc/trunk/arcadia/library/python/vault_client/vault_client/client.py?rev=r9165074#L274
	req.URL.ForceQuery = true
	return fmt.Sprintf("%s\n%s\n%s\n%d\n%s\n", strings.ToUpper(req.Method), req.URL.String(), payload,
		ts.Unix(), login), nil
}
