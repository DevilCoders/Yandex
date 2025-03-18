package storage

import (
	"context"
	"database/sql/driver"
	"errors"
	"fmt"
	"net/http"
	"strconv"

	"github.com/Masterminds/squirrel"
	"gorm.io/gorm"
	"gorm.io/gorm/clause"
	"gorm.io/gorm/schema"
)

func (v *StringArray) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v StringArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v StringArray) GormDataType() string {
	return jsonGormDataType()
}

func (v StringArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (v *HeaderOptionArray) Scan(value interface{}) error {
	var bytes []byte
	switch val := value.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", value)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v HeaderOptionArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v HeaderOptionArray) GormDataType() string {
	return jsonGormDataType()
}

func (v HeaderOptionArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (t *OriginType) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid origin type")
	}

	switch str {
	case Common:
		*t = OriginTypeCommon
	case Bucket:
		*t = OriginTypeBucket
	case Website:
		*t = OriginTypeWebsite
	default:
		return errors.New("invalid origin type")
	}

	return nil
}

func (t OriginType) Value() (driver.Value, error) {
	str := t.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown origin protocol: %d", t)
	}

	return str, nil
}

func (t *OriginType) GormDataType() string {
	return "string"
}

func (t *OriginType) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (p *OriginProtocol) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid origin protocol")
	}

	switch str {
	case HTTP:
		*p = OriginProtocolHTTP
	case HTTPS:
		*p = OriginProtocolHTTPS
	case Same:
		*p = OriginProtocolSame
	default:
		return errors.New("invalid origin protocol")
	}

	return nil
}

func (p OriginProtocol) Value() (driver.Value, error) {
	str := p.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown origin protocol: %d", p)
	}

	return str, nil
}

func (p *OriginProtocol) GormDataType() string {
	return "string"
}

func (p *OriginProtocol) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (m *CORSMode) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid cors mode")
	}

	switch str {
	case Star:
		*m = CORSModeStar
	case OriginAny:
		*m = CORSModeOriginAny
	case OriginFromList:
		*m = CORSModeOriginFromList
	default:
		return errors.New("invalid cors mode")
	}

	return nil
}

func (m CORSMode) Value() (driver.Value, error) {
	str := m.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown cors mode: %d", m)
	}

	return str, nil
}

func (m *CORSMode) GormDataType() string {
	return "string"
}

func (m *CORSMode) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (f *RewriteFlag) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid rewrite flag")
	}

	switch str {
	case Last:
		*f = RewriteFlagLast
	case Break:
		*f = RewriteFlagBreak
	case Redirect:
		*f = RewriteFlagRedirect
	case Permanent:
		*f = RewriteFlagPermanent
	default:
		return errors.New("invalid rewrite flag")
	}

	return nil
}

func (f RewriteFlag) Value() (driver.Value, error) {
	str := f.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown rewrite flag: %d", f)
	}

	return str, nil
}

func (f *RewriteFlag) GormDataType() string {
	return "string"
}

func (f *RewriteFlag) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (c *CompressCodec) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("unknown compress codec")
	}

	switch str {
	case Gzip:
		*c = CompressCodecGzip
	case Brotli:
		*c = CompressCodecBrotli
	default:
		return errors.New("invalid compress codec")
	}

	return nil
}

func (c CompressCodec) Value() (driver.Value, error) {
	str := c.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown compress codec: %d", c)
	}

	return str, nil
}

func (c *CompressCodec) GormDataType() string {
	return "string"
}

func (c *CompressCodec) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (v *CompressCodecArray) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v CompressCodecArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v *CompressCodecArray) GormDataType() string {
	return jsonGormDataType()
}

func (v *CompressCodecArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (v *OverrideTTLCodeArray) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v OverrideTTLCodeArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v OverrideTTLCodeArray) GormDataType() string {
	return jsonGormDataType()
}

func (v OverrideTTLCodeArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (v *NormalizeRequestQueryString) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v NormalizeRequestQueryString) Value() (driver.Value, error) {
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v *NormalizeRequestQueryString) GormDataType() string {
	return jsonGormDataType()
}

func (v *NormalizeRequestQueryString) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (v *CompressionVariant) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v CompressionVariant) Value() (driver.Value, error) {
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v *CompressionVariant) GormDataType() string {
	return jsonGormDataType()
}

func (v *CompressionVariant) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (t *ServeStaleErrorType) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid serve stale error type")
	}

	switch str {
	case Error:
		*t = ServeStaleError
	case Timeout:
		*t = ServeStaleTimeout
	case InvalidHeader:
		*t = ServeStaleInvalidHeader
	case Updating:
		*t = ServeStaleUpdating
	case HTTP500:
		*t = ServeStaleHTTP500
	case HTTP502:
		*t = ServeStaleHTTP502
	case HTTP503:
		*t = ServeStaleHTTP503
	case HTTP504:
		*t = ServeStaleHTTP504
	case HTTP403:
		*t = ServeStaleHTTP403
	case HTTP404:
		*t = ServeStaleHTTP404
	case HTTP429:
		*t = ServeStaleHTTP429
	default:
		return errors.New("invalid compress codec")
	}

	return nil
}

func (t ServeStaleErrorType) Value() (driver.Value, error) {
	str := t.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown serve stale error: %d", t)
	}

	return str, nil
}

func (t *ServeStaleErrorType) GormDataType() string {
	return "string"
}

func (t *ServeStaleErrorType) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (v *ServeStaleErrorArray) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v ServeStaleErrorArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v *ServeStaleErrorArray) GormDataType() string {
	return jsonGormDataType()
}

func (v *ServeStaleErrorArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func (a *HeaderAction) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid header action")
	}

	switch str {
	case Set:
		*a = HeaderActionSet
	case Append:
		*a = HeaderActionAppend
	case Remove:
		*a = HeaderActionRemove
	default:
		return errors.New("invalid compress codec")
	}

	return nil
}

func (a HeaderAction) Value() (driver.Value, error) {
	str := a.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown header action: %d", a)
	}

	return str, nil
}

func (a *HeaderAction) GormDataType() string {
	return "string"
}

func (a *HeaderAction) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (m *AllowedMethod) Scan(src interface{}) error {
	str, ok := src.(string)
	if !ok {
		return errors.New("invalid allowed method")
	}

	switch str {
	case http.MethodGet:
		*m = AllowedMethodGet
	case http.MethodHead:
		*m = AllowedMethodHead
	case http.MethodPost:
		*m = AllowedMethodPost
	case http.MethodPut:
		*m = AllowedMethodPut
	case http.MethodPatch:
		*m = AllowedMethodPatch
	case http.MethodDelete:
		*m = AllowedMethodDelete
	case http.MethodOptions:
		*m = AllowedMethodOptions
	default:
		return errors.New("invalid allowed methods")
	}

	return nil
}

func (m AllowedMethod) Value() (driver.Value, error) {
	str := m.String()
	if str == Unspecified {
		return nil, fmt.Errorf("unknown allowed method: %d", m)
	}

	return str, nil
}

func (m *AllowedMethod) GormDataType() string {
	return "string"
}

func (m *AllowedMethod) GormDBDataType(_ *gorm.DB, _ *schema.Field) string {
	return "text"
}

func (v *AllowedMethodArray) Scan(src interface{}) error {
	var bytes []byte
	switch val := src.(type) {
	case []byte:
		bytes = val
	case string:
		bytes = []byte(val)
	default:
		return fmt.Errorf("failed to unmarshal JSONB value: %v", src)
	}

	if err := v.UnmarshalJSON(bytes); err != nil {
		return err
	}

	return nil
}

func (v AllowedMethodArray) Value() (driver.Value, error) {
	if v == nil {
		return nil, nil
	}
	json, err := v.MarshalJSON()
	return string(json), err
}

func (v *AllowedMethodArray) GormDataType() string {
	return jsonGormDataType()
}

func (v *AllowedMethodArray) GormDBDataType(db *gorm.DB, field *schema.Field) string {
	return jsonGormDBDataType(db, field)
}

func jsonGormDataType() string {
	return "json"
}

func jsonGormDBDataType(db *gorm.DB, _ *schema.Field) string {
	switch db.Dialector.Name() {
	case "sqlite", "mysql":
		return "JSON"
	case "postgres":
		return "JSONB"
	default:
		return ""
	}
}

func (v EntityVersion) GormValue(ctx context.Context, db *gorm.DB) clause.Expr {
	exprStub := clause.Expr{
		SQL:                "0",
		Vars:               nil,
		WithoutParentheses: false,
	}

	if v != 0 {
		exprStub.SQL = strconv.FormatInt(int64(v), 10)
		return exprStub
	}

	table := db.Statement.Table

	var entityID interface{}
	switch m := db.Statement.Model.(type) {
	case *OriginsGroupEntity:
		entityID = m.EntityID
	case *ResourceEntity:
		entityID = m.EntityID
	default:
		return exprStub
	}

	// TODO: names
	sql, args, err := squirrel.Select("entity_version + 1").
		From(table).
		Where(squirrel.Eq{
			"entity_id": entityID,
		}).
		OrderBy("entity_version DESC").
		Limit(1).
		ToSql()
	if err != nil {
		return exprStub
	}

	return clause.Expr{
		SQL:                "(" + sql + ")",
		Vars:               args,
		WithoutParentheses: false,
	}
}
