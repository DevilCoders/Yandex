package main

import (
	"bufio"
	"context"
	"crypto/ecdsa"
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang-jwt/jwt/v4"

	device_validator_pb "a.yandex-team.ru/antirobot/device_validator/proto"
)

type signingKey struct {
	keyIssuer string
	keyID     string
	key       *ecdsa.PrivateKey

	signatureMutex sync.RWMutex
	signature      string
	signatureTime  time.Time
}

type iosAPI struct {
	env         *Env
	httpClient  *http.Client
	signingKeys map[string]*signingKey
}

func InstallIOSAPI(env *Env, mux *http.ServeMux) error {
	api := &iosAPI{env: env}

	api.httpClient = &http.Client{
		Timeout: 10 * time.Second,
	}

	err := api.readSigningKey(env.Args.DeviceCheckKey)
	if err != nil {
		return fmt.Errorf("failed to read DeviceCheck key file: %w", err)
	}

	mux.HandleFunc("/ios/authenticate", api.handleIOSAuthenticate)

	return nil
}

func (api *iosAPI) readSigningKey(path string) error {
	signingKeyFile, err := os.Open(path)
	if err != nil {
		return fmt.Errorf("open: %w", err)
	}

	appIDToKeyConfig := make(map[string]struct {
		KeyIssuer string `json:"iss"`
		KeyID     string `json:"kid"`
		Key       string `json:"key"`
	})

	signingKeyReader := bufio.NewReader(signingKeyFile)
	err = json.NewDecoder(signingKeyReader).Decode(&appIDToKeyConfig)
	if err != nil {
		return fmt.Errorf("JSON decode: %w", err)
	}

	if _, ok := appIDToKeyConfig["default"]; !ok {
		return fmt.Errorf("missing default key")
	}

	api.signingKeys = make(map[string]*signingKey)

	for appID, config := range appIDToKeyConfig {
		key := &signingKey{
			keyIssuer: config.KeyIssuer,
			keyID:     config.KeyID,
		}

		api.signingKeys[appID] = key

		encodedSigningKey, err := base64.StdEncoding.DecodeString(config.Key)
		if err != nil {
			return fmt.Errorf("key base64 decode: %w", err)
		}

		key.key, err = DecodeECDSAPrivateKey(encodedSigningKey)
		if err != nil {
			return fmt.Errorf("key decode: %w", err)
		}
	}

	return nil
}

func (api *iosAPI) handleIOSAuthenticate(writer http.ResponseWriter, request *http.Request) {
	SetJSONContentType(writer)

	ip := GetUserIP(request)
	now := time.Now()
	nowMs := uint64(now.UnixNano() / 1000000)

	logRecord := &device_validator_pb.TIosRequest{
		Ip:               &ip,
		RequestTimestamp: &nowMs,
	}

	defer api.env.Log(&device_validator_pb.TLogRecord{IosRequest: logRecord})

	transactionID, transactionIDErr := makeTransactionID()
	logRecord.TransactionIdError = ErrorStringPtr(transactionIDErr)
	if transactionIDErr != nil {
		api.env.Stats.ServerErrors.Inc()
	} else {
		logRecord.TransactionId = &transactionID
	}

	var body struct {
		Timestamp   uint64 `json:"timestamp"`
		Attestation string `json:"attestation"`
		UUID        string `json:"uuid"`
		AppID       string `json:"bundleId"`
	}

	err := json.NewDecoder(request.Body).Decode(&body)
	logRecord.ValidBody = BoolPtr(err == nil)
	if err != nil {
		api.env.Stats.GetInvalidAppIDStats().IOSRejections.Inc()
		WriteErrorResponse(writer, fmt.Errorf("invalid JSON body: %w", err))
		return
	}

	logRecord.Attestation = &body.Attestation
	logRecord.Uuid = &body.UUID
	logRecord.UserTimestamp = &body.Timestamp
	logRecord.AppId = &body.AppID

	appIDStats := api.env.Stats.FindAppIDStats(body.AppID)

	if transactionIDErr != nil {
		WriteErrorResponse(
			writer,
			fmt.Errorf("failed to generate transaction ID: %w", transactionIDErr),
		)
		return
	}

	passed, err := api.performDeviceCheck(request.Context(), transactionID, now, body.Attestation, body.AppID)
	logRecord.Passed = &passed
	logRecord.DeviceCheckError = ErrorStringPtr(err)
	if !passed {
		appIDStats.IOSRejections.Inc()
		WriteErrorResponse(writer, err)
		return
	}

	if err != nil {
		appIDStats.IOSFailedAppleRequests.Inc()
	}

	expiresAtMs := nowMs + api.env.Args.MaxTokenAgeMs
	logRecord.JwtExpirationTimestamp = &expiresAtMs
	ourClaims := jwt.MapClaims{
		"timestamp_ms":     nowMs,
		"expires_at_ms":    expiresAtMs,
		"device_integrity": true,
		"ip":               ip,
		"uuid":             body.UUID,
	}

	err = WriteTokenResponse(
		writer,
		api.env.TokenSigningKeyID,
		api.env.TokenSigningMethod,
		api.env.TokenSigningKey,
		ourClaims,
	)
	logRecord.ResponseError = ErrorStringPtr(err)

	appIDStats.IOSPasses.Inc()
}

func (api *iosAPI) performDeviceCheck(
	context context.Context,
	transactionID string,
	now time.Time,
	attestation string,
	appID string,
) (bool, error) {
	signingKey, ok := api.signingKeys[appID]
	if !ok {
		signingKey = api.signingKeys["default"]
	}

	deviceCheckTokenStr, err := api.getSignature(now, signingKey)
	if err != nil {
		return true, err
	}

	deviceCheckBody := &strings.Builder{}

	_ = json.NewEncoder(deviceCheckBody).Encode(map[string]interface{}{
		"device_token":   attestation,
		"timestamp":      now.UnixNano() / 1000000,
		"transaction_id": transactionID,
	})

	deviceCheckBodyReader := strings.NewReader(deviceCheckBody.String())

	deviceCheckReq, err := http.NewRequestWithContext(
		context,
		"POST", JoinURL(api.env.Args.DeviceCheckAPI, "v1/validate_device_token"),
		deviceCheckBodyReader,
	)
	if err != nil {
		return true, fmt.Errorf("failed to create a request: %w", err)
	}

	deviceCheckReq.Header.Add("Authorization", "Bearer "+deviceCheckTokenStr)

	deviceCheckResp, err := api.httpClient.Do(deviceCheckReq)
	if err != nil {
		return true, fmt.Errorf("failed to perform a DeviceCheck request: %w", err)
	}

	defer deviceCheckResp.Body.Close()

	if deviceCheckResp.StatusCode != http.StatusOK {
		msg, _ := io.ReadAll(deviceCheckResp.Body)
		description := " " + string(msg)

		passed := deviceCheckResp.StatusCode != http.StatusBadRequest

		err = fmt.Errorf(
			"failed to perform a DeviceCheck request: %d%s",
			deviceCheckResp.StatusCode, description,
		)

		return passed, err
	}

	return true, nil
}

func (api *iosAPI) getSignature(now time.Time, key *signingKey) (string, error) {
	key.signatureMutex.RLock()

	refreshInterval := time.Duration(api.env.Args.DeviceCheckRefreshIntervalMs) * time.Millisecond
	if now.Sub(key.signatureTime) > refreshInterval {
		key.signatureMutex.RUnlock()

		key.signatureMutex.Lock()
		defer key.signatureMutex.Unlock()

		// This check is not useless because another goroutine could have done the update for us
		// if it acquired signatureMutex between RUnlock() and Lock().
		now = time.Now()
		if now.Sub(key.signatureTime) > refreshInterval {
			newSignature, err := key.makeSignature(now)
			if err != nil {
				return "", err
			}

			key.signature = newSignature
			key.signatureTime = now
		}

		return key.signature, nil
	} else {
		defer key.signatureMutex.RUnlock()
		return key.signature, nil
	}
}

func (key *signingKey) makeSignature(now time.Time) (string, error) {
	deviceCheckClaims := jwt.MapClaims{
		"iss": key.keyIssuer,
		"iat": now.Unix(),
	}

	deviceCheckToken := jwt.NewWithClaims(jwt.SigningMethodES256, deviceCheckClaims)
	deviceCheckToken.Header["kid"] = key.keyID
	deviceCheckTokenStr, err := deviceCheckToken.SignedString(key.key)
	if err != nil {
		return "", fmt.Errorf("failed to sign DeviceCheck token: %w", err)
	}

	return deviceCheckTokenStr, nil
}

func makeTransactionID() (string, error) {
	buf := make([]byte, 16)
	_, err := rand.Read(buf)
	if err != nil {
		return "", err
	}

	id, _ := uuid.FromBytes(buf)
	return id.String(), nil
}
