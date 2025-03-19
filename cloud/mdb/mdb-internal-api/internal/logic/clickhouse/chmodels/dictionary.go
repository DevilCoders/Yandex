package chmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

const (
	DefaultMongoDBPort          = int64(27017)
	DefaultClickHousePort       = int64(9000)
	DefaultPostgreSQLPort       = int64(5432)
	DefaultMySQLReplicaPriority = int64(100)
)

type Dictionary struct {
	Name             string
	Structure        DictionaryStructure
	Layout           DictionaryLayout
	FixedLifetime    optional.Int64
	LifetimeRange    DictionaryLifetimeRange
	HTTPSource       DictionarySourceHTTP
	MySQLSource      DictionarySourceMySQL
	ClickhouseSource DictionarySourceClickhouse
	MongoDBSource    DictionarySourceMongoDB
	PostgreSQLSource DictionarySourcePostgreSQL
	YTSource         DictionarySourceYT
}

type DictionaryLifetimeRange struct {
	Min int64
	Max int64

	Valid bool
}

type DictionaryLayout struct {
	Type        DictionaryLayoutType
	SizeInCells optional.Int64
}

type DictionaryLayoutType string

const (
	DictionaryLayoutTypeFlat             = DictionaryLayoutType("flat")
	DictionaryLayoutTypeHashed           = DictionaryLayoutType("hashed")
	DictionaryLayoutTypeComplexKeyHashed = DictionaryLayoutType("complex_key_hashed")
	DictionaryLayoutTypeRangeHashed      = DictionaryLayoutType("range_hashed")
	DictionaryLayoutTypeCache            = DictionaryLayoutType("cache")
	DictionaryLayoutTypeComplexKeyCache  = DictionaryLayoutType("complex_key_cache")
)

type DictionaryStructure struct {
	ID         optional.String
	Key        DictionaryStructureKey
	RangeMin   DictionaryStructureAttribute
	RangeMax   DictionaryStructureAttribute
	Attributes []DictionaryStructureAttribute
}

type DictionaryStructureKey struct {
	Attributes []DictionaryStructureAttribute

	Valid bool
}

type DictionaryStructureAttribute struct {
	Name         string
	Type         string
	NullValue    optional.String
	Expression   optional.String
	Hierarchical optional.Bool
	Injective    optional.Bool

	Valid bool
}

type DictionarySourceHTTP struct {
	URL    string
	Format string

	Valid bool
}

type DictionarySourceMySQL struct {
	DB              string
	Table           string
	Port            optional.Int64
	User            optional.String
	Password        optional.OptionalPassword
	Replicas        []DictionarySourceMySQLReplica
	Where           optional.String
	InvalidateQuery optional.String

	Valid bool
}

type DictionarySourceMySQLReplica struct {
	Host     string
	Priority optional.Int64
	Port     optional.Int64
	User     optional.String
	Password optional.OptionalPassword
}

type DictionarySourceClickhouse struct {
	DB       string
	Table    string
	Host     string
	Port     optional.Int64
	User     string
	Password optional.OptionalPassword
	Where    optional.String

	Valid bool
}

type DictionarySourceMongoDB struct {
	DB         string
	Collection string
	Host       string
	Port       optional.Int64
	User       string
	Password   optional.OptionalPassword

	Valid bool
}

type DictionarySourcePostgreSQL struct {
	DB              string
	Table           string
	Hosts           []string
	Port            optional.Int64
	User            string
	Password        optional.OptionalPassword
	InvalidateQuery optional.String
	SSLMode         PostgresSSLMode

	Valid bool
}

type PostgresSSLMode optional.String

var (
	PostgresSSLModeUnspecified = PostgresSSLMode{}
	PostgresSSLModeDisable     = PostgresSSLMode(optional.NewString("DISABLE"))
	PostgresSSLModeAllow       = PostgresSSLMode(optional.NewString("ALLOW"))
	PostgresSSLModePrefer      = PostgresSSLMode(optional.NewString("PREFER"))
	PostgresSSLModeVerifyCA    = PostgresSSLMode(optional.NewString("VERIFY_CA"))
	PostgresSSLModeVerifyFull  = PostgresSSLMode(optional.NewString("VERIFY_FULL"))
)

type DictionarySourceYT struct {
	Clusters            []string
	Table               string
	Keys                []string
	Fields              []string
	DateFields          []string
	DatetimeFields      []string
	Query               optional.String
	User                optional.String
	Token               optional.OptionalPassword
	ClusterSelection    YTClusterSelection
	UseQueryForCache    optional.Bool
	ForceReadTable      optional.Bool
	RangeExpansionLimit optional.Int64
	InputRowLimit       optional.Int64
	OutputRowLimit      optional.Int64
	YTSocketTimeout     optional.Int64
	YTConnectionTimeout optional.Int64
	YTLookupTimeout     optional.Int64
	YTSelectTimeout     optional.Int64
	YTRetryCount        optional.Int64

	Valid bool
}

type YTClusterSelection optional.String

var (
	YTClusterSelectionUnspecified = YTClusterSelection{}
	YTClusterSelectionOrdered     = YTClusterSelection(optional.NewString("Ordered"))
	YTClusterSelectionRandom      = YTClusterSelection(optional.NewString("Random"))
)

func (d Dictionary) Validate() error {
	if err := models.ClusterNameValidator.ValidateString(d.Name); err != nil {
		return err
	}

	if !d.FixedLifetime.Valid && !d.LifetimeRange.Valid {
		return semerr.InvalidInputf("dictionary %q lifetime is not specififed", d.Name)
	}

	if !(d.HTTPSource.Valid || d.MySQLSource.Valid || d.ClickhouseSource.Valid || d.MongoDBSource.Valid || d.PostgreSQLSource.Valid || d.YTSource.Valid) {
		return semerr.InvalidInputf("dictionary %q source is not specified", d.Name)
	}

	isComplexLayout := d.Layout.Type == DictionaryLayoutTypeComplexKeyCache || d.Layout.Type == DictionaryLayoutTypeComplexKeyHashed
	if d.Structure.ID.Valid && isComplexLayout {
		return semerr.InvalidInputf("the layout %q cannot be used for numeric keys.", d.Layout.Type)
	}

	if !d.Structure.ID.Valid && !isComplexLayout {
		return semerr.InvalidInputf("the layout %q cannot be used for complex keys.", d.Layout.Type)
	}

	if d.Layout.Type == DictionaryLayoutTypeRangeHashed {
		if !d.Structure.RangeMin.Valid {
			return semerr.InvalidInputf("range min must be declared for dictionaries with range hashed layout.")
		}
		if !d.Structure.RangeMax.Valid {
			return semerr.InvalidInputf("range max must be declared for dictionaries with range hashed layout.")
		}
	} else {
		if d.Structure.RangeMin.Valid {
			return semerr.InvalidInputf("range min must not be declared for dictionaries with layout other than range hashed.")
		}
		if d.Structure.RangeMax.Valid {
			return semerr.InvalidInputf("range max must not be declared for dictionaries with layout other than range hashed.")
		}
	}

	if err := d.Structure.Validate(); err != nil {
		return err
	}

	return d.Layout.Validate()
}

func (l DictionaryLayout) Validate() error {
	if l.Type == DictionaryLayoutTypeCache || l.Type == DictionaryLayoutTypeComplexKeyCache {
		if !l.SizeInCells.Valid {
			return semerr.InvalidInputf("the parameter 'size_in_cells' is required for the layout %q", l.Type)
		}
	} else {
		if l.SizeInCells.Valid {
			return semerr.InvalidInputf("the parameter 'size_in_cells' is not applicable for the layout %q", l.Type)
		}
	}

	return nil
}

func (s DictionaryStructure) Validate() error {
	if len(s.Attributes) == 0 {
		return semerr.InvalidInputf("attributes length is shorted than minimum 1")
	}

	if s.ID.Valid && s.Key.Valid {
		return semerr.InvalidInputf("the parameters 'id' and 'key' are mutually exclusive.")
	}

	if !s.ID.Valid && !s.Key.Valid {
		return semerr.InvalidInputf("either 'id' or 'key' must be specified.")
	}

	return nil
}
