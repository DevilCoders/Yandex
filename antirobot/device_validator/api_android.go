package main

import (
	"bufio"
	"crypto/hmac"
	"crypto/rand"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"time"

	"github.com/golang-jwt/jwt/v4"

	"a.yandex-team.ru/antirobot/device_validator/proto"
)

const NonceHMACKeySize = 32
const nonceRandomBytesSize = 16

var keptAndroidClaims = []string{
	"apkPackageName",
	"apkCertificateDigestSha256",
	"basicIntegrity",
	"ctsProfileMatch",
	"evaluationType",
	"timestampMs",
}

type androidAPI struct {
	env                  *Env
	defaultServiceConfig *ServiceConfig
	certFingerprints     map[string]struct{}
}

func InstallAndroidAPI(env *Env, mux *http.ServeMux) error {
	var err error

	api := androidAPI{env: env}

	api.defaultServiceConfig = &ServiceConfig{
		MaxTokenAgeMs:        env.Args.MaxTokenAgeMs,
		ExcludeAndroidClaims: false,
	}

	if len(env.Args.APKCertFingerprints) > 0 {
		err = api.loadFingerprints(env.Args.APKCertFingerprints)
		if err != nil {
			return fmt.Errorf("failed to load fingerprints: %w", err)
		}
	}

	mux.HandleFunc("/android/generate_nonce", api.handleAndroidGenerateNonce)
	mux.HandleFunc("/android/authenticate", api.handleAndroidAuthenticate)

	return nil
}

func (api *androidAPI) loadFingerprints(path string) error {
	api.certFingerprints = make(map[string]struct{})

	file, err := os.Open(path)
	if err != nil {
		return err
	}

	scanner := bufio.NewScanner(file)

	for i := 0; scanner.Scan(); i += 1 {
		fingerprintStr := scanner.Text()
		if len(fingerprintStr) == 0 {
			continue
		}

		fingerprint, err := ParseFingerprint(fingerprintStr)
		if err != nil {
			return fmt.Errorf("bad fingerprint at index %d: %w", i, err)
		}

		api.certFingerprints[string(fingerprint)] = struct{}{}
	}

	return scanner.Err()
}

func (api *androidAPI) handleAndroidGenerateNonce(
	writer http.ResponseWriter,
	request *http.Request,
) {
	SetJSONContentType(writer)

	ip := GetUserIP(request)
	nowMs := uint64(time.Now().UnixNano() / 1000000)
	logRecord := &device_validator_pb.TAndroidNonceRequest{
		Ip:               &ip,
		RequestTimestamp: &nowMs,
	}

	defer api.env.Log(&device_validator_pb.TLogRecord{AndroidNonceRequest: logRecord})

	var body struct {
		UUID  string `json:"uuid"`
		AppID string `json:"app_id"`
	}

	err := json.NewDecoder(request.Body).Decode(&body)
	logRecord.ValidBody = BoolPtr(err == nil)
	if err != nil {
		api.env.Stats.GetInvalidAppIDStats().AndroidNonceRequests.Inc()
		WriteErrorResponse(writer, fmt.Errorf("invalid JSON body: %w", err))
		return
	}

	logRecord.Uuid = &body.UUID
	logRecord.AppId = &body.AppID

	appIDStats := api.env.Stats.FindAppIDStats(body.AppID)
	appIDStats.AndroidNonceRequests.Inc()

	randomBytes := make([]byte, nonceRandomBytesSize)
	_, err = rand.Read(randomBytes)
	if err != nil {
		api.env.Stats.ServerErrors.Inc()
		WriteErrorResponse(writer, fmt.Errorf("rand.Read failed: %w", err))
		return
	}

	nonce := api.generateNonceTail(randomBytes, body.UUID, randomBytes)

	_ = json.NewEncoder(writer).Encode(map[string]interface{}{
		"nonce": nonce,
	})
}

func (api *androidAPI) generateNonceTail(randomBytes []byte, uuid string, head []byte) []byte {
	h := hmac.New(sha256.New, api.env.NonceKey)
	h.Write(randomBytes)
	h.Write([]byte(uuid))
	return h.Sum(head)
}

func (api *androidAPI) handleAndroidAuthenticate(
	writer http.ResponseWriter,
	request *http.Request,
) {
	SetJSONContentType(writer)

	ip := GetUserIP(request)
	nowMs := uint64(time.Now().UnixNano() / 1000000)

	logRecord := &device_validator_pb.TAndroidAuthenticateRequest{
		Ip:               &ip,
		RequestTimestamp: &nowMs,
	}

	defer api.env.Log(&device_validator_pb.TLogRecord{AndroidAuthenticateRequest: logRecord})

	var body struct {
		Attestation string `json:"attestation"`
		UUID        string `json:"uuid"`
	}

	err := json.NewDecoder(request.Body).Decode(&body)
	logRecord.ValidBody = BoolPtr(err == nil)
	if err != nil {
		api.env.Stats.GetInvalidAppIDStats().AndroidRejections.Inc()
		WriteErrorResponse(writer, fmt.Errorf("invalid JSON body: %w", err))
		return
	}

	logRecord.Attestation = &body.Attestation
	logRecord.Uuid = &body.UUID

	claims, err := CheckSafetyNetAttestation(body.Attestation)
	copyClaimsToAndroidAuthenticateRequest(claims, logRecord)
	logRecord.Passed = BoolPtr(err == nil)
	logRecord.SafetynetError = ErrorStringPtr(err)
	appIDStats := api.findAppIDStatsByClaims(claims)
	if err != nil {
		appIDStats.AndroidRejections.Inc()
		WriteErrorResponse(writer, fmt.Errorf("invalid attestation: %w", err))
		return
	}

	err = api.checkClaims(body.UUID, nowMs, claims)
	logRecord.Passed = BoolPtr(err == nil)
	logRecord.ClaimError = ErrorStringPtr(err)
	if err != nil {
		appIDStats.AndroidRejections.Inc()
		WriteErrorResponse(writer, err)
		return
	}

	serviceConfig := api.findServiceConfig(claims)

	deviceIntegrity := (claims["ctsProfileMatch"] == true) && (claims["basicIntegrity"] == true)

	cleanClaims := make(map[string]interface{})

	if !serviceConfig.ExcludeAndroidClaims {
		for _, key := range keptAndroidClaims {
			if value, ok := claims[key]; ok {
				cleanClaims[key] = value
			}
		}
	}

	expirationTimestampMs := nowMs + serviceConfig.MaxTokenAgeMs
	logRecord.JwtExpirationTimestamp = &expirationTimestampMs

	ourClaims := jwt.MapClaims{
		"android_claims":   cleanClaims,
		"timestamp_ms":     nowMs,
		"expires_at_ms":    expirationTimestampMs,
		"device_integrity": deviceIntegrity,
		"ip":               ip,
		"uuid":             body.UUID,
	}

	if len(api.certFingerprints) > 0 {
		ourClaims["has_valid_fingerprint"] = checkFingerprints(api.certFingerprints, claims)
	}

	err = WriteTokenResponse(
		writer,
		api.env.TokenSigningKeyID,
		api.env.TokenSigningMethod,
		api.env.TokenSigningKey,
		ourClaims,
	)
	logRecord.ResponseError = ErrorStringPtr(err)

	appIDStats.AndroidPasses.Inc()
}

func (api *androidAPI) findAppIDStatsByClaims(claims jwt.MapClaims) *AppIDStats {
	var appIDStats *AppIDStats
	if appID, ok := claims["apkPackageName"].(string); ok {
		appIDStats = api.env.Stats.ByAppID[appID]
	}

	if appIDStats == nil {
		appIDStats = api.env.Stats.GetInvalidAppIDStats()
	}

	return appIDStats
}

func (api *androidAPI) checkClaims(
	uuid string,
	nowMs uint64,
	claims jwt.MapClaims,
) error {
	if api.env.Args.MaxAttestationAgeMs != 0 &&
		!checkTimestamp(claims, api.env.Args.MaxAttestationAgeMs, nowMs) {

		return errors.New("invalid attestation timestamp")
	}

	nonce, err := GetJSONBytes(claims["nonce"])
	if err != nil {
		return fmt.Errorf("invalid nonce: %w", err)
	}

	expectedNonceTail := api.generateNonceTail(nonce[:nonceRandomBytesSize], uuid, []byte{})
	if subtle.ConstantTimeCompare(nonce[nonceRandomBytesSize:], expectedNonceTail) != 1 {
		return fmt.Errorf("invalid nonce")
	}

	return nil
}

func (api *androidAPI) findServiceConfig(claims jwt.MapClaims) *ServiceConfig {
	appID, hasAppID := claims["apkPackageName"].(string)
	if !hasAppID {
		return api.defaultServiceConfig
	}

	serviceConfig := api.env.AppIDToConfig[appID]
	if serviceConfig == nil {
		return api.defaultServiceConfig
	}

	return serviceConfig
}

func copyClaimsToAndroidAuthenticateRequest(
	claims jwt.MapClaims,
	logRecord *device_validator_pb.TAndroidAuthenticateRequest,
) {
	if appID, ok := claims["apkPackageName"].(string); ok {
		logRecord.AppId = &appID
	}

	if certDigest, ok := claims["apkCertificateDigestSha256"].(string); ok {
		logRecord.ApkCertificateDigestSha256 = &certDigest
	}

	if ctsProfileMatch, ok := claims["ctsProfileMatch"].(bool); ok {
		logRecord.CtsProfileMatch = &ctsProfileMatch
	}

	if basicIntegrity, ok := claims["basicIntegrity"].(bool); ok {
		logRecord.BasicIntegrity = &basicIntegrity
	}

	if timestampFlt, ok := claims["timestampMs"].(float64); ok {
		timestamp := uint64(timestampFlt)
		logRecord.SafetynetTimestamp = &timestamp
	}
}

func checkTimestamp(claims jwt.MapClaims, maxAttestationAgeMs uint64, nowMs uint64) bool {
	attestationTimestampMsFlt, ok := claims["timestampMs"].(float64)
	if !ok {
		return false
	}

	attestationTimestampMs := uint64(attestationTimestampMsFlt)

	return attestationTimestampMs <= nowMs && nowMs-attestationTimestampMs <= maxAttestationAgeMs
}

func checkFingerprints(certFingerprints map[string]struct{}, claims jwt.MapClaims) bool {
	fingerprintsUntyped, ok := claims["apkCertificateDigestSha256"]
	if !ok {
		return false
	}

	fingerprints, ok := fingerprintsUntyped.([]string)
	if !ok {
		return false
	}

	for _, fingerprintEnc := range fingerprints {
		fingerprint, err := base64.StdEncoding.DecodeString(fingerprintEnc)
		if err == nil {
			if _, ok := certFingerprints[string(fingerprint)]; ok {
				return true
			}
		}
	}

	return false
}
