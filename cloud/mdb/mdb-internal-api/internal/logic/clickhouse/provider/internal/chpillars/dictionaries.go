package chpillars

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

type Dictionary struct {
	Name             string                      `json:"name,omitempty"`
	Structure        DictionaryStructure         `json:"structure,omitempty"`
	Layout           DictionaryLayout            `json:"layout,omitempty"`
	FixedLifetime    *int64                      `json:"fixed_lifetime,omitempty"`
	LifetimeRange    *DictionaryRange            `json:"lifetime_range,omitempty"`
	HTTPSource       *DictionarySourceHTTP       `json:"http_source,omitempty"`
	MySQLSource      *DictionarySourceMySQL      `json:"mysql_source,omitempty"`
	ClickhouseSource *DictionarySourceClickhouse `json:"clickhouse_source,omitempty"`
	MongoDBSource    *DictionarySourceMongoDB    `json:"mongodb_source,omitempty"`
	PostgreSQLSource *DictionarySourcePostgreSQL `json:"postgresql_source,omitempty"`
	YTSource         *DictionarySourceYT         `json:"yt_source,omitempty"`
}

func (d Dictionary) toModel() chmodels.Dictionary {
	return chmodels.Dictionary{
		Name:             d.Name,
		Structure:        d.Structure.toModel(),
		Layout:           d.Layout.toModel(),
		FixedLifetime:    pillars.MapPtrInt64ToOptionalInt64(d.FixedLifetime),
		LifetimeRange:    d.LifetimeRange.toModel(),
		HTTPSource:       d.HTTPSource.toModel(),
		MySQLSource:      d.MySQLSource.toModel(),
		ClickhouseSource: d.ClickhouseSource.toModel(),
		MongoDBSource:    d.MongoDBSource.toModel(),
		PostgreSQLSource: d.PostgreSQLSource.toModel(),
		YTSource:         d.YTSource.toModel(),
	}
}

func dictionariesToModel(d []Dictionary) []chmodels.Dictionary {
	res := make([]chmodels.Dictionary, len(d))
	for i, dictionary := range d {
		res[i] = dictionary.toModel()
	}
	return res
}

func dictionaryFromModel(cryptoProvider crypto.Crypto, d chmodels.Dictionary) (Dictionary, error) {
	dict := Dictionary{
		Name:          d.Name,
		FixedLifetime: pillars.MapOptionalInt64ToPtrInt64(d.FixedLifetime),
		Structure:     structureFromModel(d.Structure),
		Layout: DictionaryLayout{
			Type:        d.Layout.Type,
			SizeInCells: pillars.MapOptionalInt64ToPtrInt64(d.Layout.SizeInCells),
		},
	}

	if d.LifetimeRange.Valid {
		dict.LifetimeRange = &DictionaryRange{
			Min: d.LifetimeRange.Min,
			Max: d.LifetimeRange.Max,
		}
	}

	switch {
	case d.HTTPSource.Valid:
		dict.HTTPSource = &DictionarySourceHTTP{
			URL:    d.HTTPSource.URL,
			Format: d.HTTPSource.Format,
		}
	case d.MySQLSource.Valid:
		replicas := make([]DictionarySourceMySQLReplica, 0, len(d.MySQLSource.Replicas))
		for _, r := range d.MySQLSource.Replicas {
			pass, err := EncryptOptionalPassword(cryptoProvider, r.Password)
			if err != nil {
				return Dictionary{}, err
			}

			replicas = append(replicas, DictionarySourceMySQLReplica{
				Host:     r.Host,
				Priority: pillars.MapOptionalInt64ToPtrInt64(r.Priority),
				Port:     pillars.MapOptionalInt64ToPtrInt64(r.Port),
				User:     pillars.MapOptionalStringToPtrString(r.User),
				Password: pass,
			})
		}

		pass, err := EncryptOptionalPassword(cryptoProvider, d.MySQLSource.Password)
		if err != nil {
			return Dictionary{}, err
		}

		dict.MySQLSource = &DictionarySourceMySQL{
			DB:              d.MySQLSource.DB,
			Table:           d.MySQLSource.Table,
			Port:            pillars.MapOptionalInt64ToPtrInt64(d.MySQLSource.Port),
			User:            pillars.MapOptionalStringToPtrString(d.MySQLSource.User),
			Replicas:        replicas,
			Password:        pass,
			Where:           pillars.MapOptionalStringToPtrString(d.MySQLSource.Where),
			InvalidateQuery: pillars.MapOptionalStringToPtrString(d.MySQLSource.InvalidateQuery),
		}
	case d.ClickhouseSource.Valid:
		pass, err := EncryptOptionalPassword(cryptoProvider, d.ClickhouseSource.Password)
		if err != nil {
			return Dictionary{}, err
		}

		dict.ClickhouseSource = &DictionarySourceClickhouse{
			DB:       d.ClickhouseSource.DB,
			Table:    d.ClickhouseSource.Table,
			Host:     d.ClickhouseSource.Host,
			Port:     pillars.MapOptionalInt64ToPtrInt64(d.ClickhouseSource.Port),
			User:     d.ClickhouseSource.User,
			Password: pass,
			Where:    pillars.MapOptionalStringToPtrString(d.ClickhouseSource.Where),
		}
	case d.MongoDBSource.Valid:
		pass, err := EncryptOptionalPassword(cryptoProvider, d.MongoDBSource.Password)
		if err != nil {
			return Dictionary{}, err
		}

		dict.MongoDBSource = &DictionarySourceMongoDB{
			DB:         d.MongoDBSource.DB,
			Collection: d.MongoDBSource.Collection,
			Host:       d.MongoDBSource.Host,
			Port:       pillars.MapOptionalInt64ToPtrInt64(d.MongoDBSource.Port),
			User:       d.MongoDBSource.User,
			Password:   pass,
		}
	case d.PostgreSQLSource.Valid:
		pass, err := EncryptOptionalPassword(cryptoProvider, d.PostgreSQLSource.Password)
		if err != nil {
			return Dictionary{}, err
		}

		dict.PostgreSQLSource = &DictionarySourcePostgreSQL{
			DB:              d.PostgreSQLSource.DB,
			Table:           d.PostgreSQLSource.Table,
			Hosts:           d.PostgreSQLSource.Hosts,
			Port:            pillars.MapOptionalInt64ToPtrInt64(d.PostgreSQLSource.Port),
			User:            d.PostgreSQLSource.User,
			Password:        pass,
			InvalidateQuery: pillars.MapOptionalStringToPtrString(d.PostgreSQLSource.InvalidateQuery),
			SSLMode:         pillars.MapOptionalStringToPtrString(optional.String(d.PostgreSQLSource.SSLMode)),
		}
	case d.YTSource.Valid:
		token, err := EncryptOptionalPassword(cryptoProvider, d.YTSource.Token)
		if err != nil {
			return Dictionary{}, err
		}

		dict.YTSource = &DictionarySourceYT{
			Clusters:            d.YTSource.Clusters,
			Table:               d.YTSource.Table,
			Keys:                d.YTSource.Keys,
			Fields:              d.YTSource.Fields,
			DateFields:          d.YTSource.DateFields,
			DatetimeFields:      d.YTSource.DatetimeFields,
			Query:               pillars.MapOptionalStringToPtrString(d.YTSource.Query),
			User:                pillars.MapOptionalStringToPtrString(d.YTSource.User),
			Token:               token,
			ClusterSelection:    pillars.MapOptionalStringToPtrString(optional.String(d.YTSource.ClusterSelection)),
			UseQueryForCache:    pillars.MapOptionalBoolToPtrBool(d.YTSource.UseQueryForCache),
			ForceReadTable:      pillars.MapOptionalBoolToPtrBool(d.YTSource.ForceReadTable),
			RangeExpansionLimit: pillars.MapOptionalInt64ToPtrInt64(d.YTSource.RangeExpansionLimit),
			InputRowLimit:       pillars.MapOptionalInt64ToPtrInt64(d.YTSource.InputRowLimit),
			OutputRowLimit:      pillars.MapOptionalInt64ToPtrInt64(d.YTSource.OutputRowLimit),
			YTSocketTimeout:     pillars.MapOptionalInt64ToPtrInt64(d.YTSource.YTSocketTimeout),
			YTConnectionTimeout: pillars.MapOptionalInt64ToPtrInt64(d.YTSource.YTConnectionTimeout),
			YTLookupTimeout:     pillars.MapOptionalInt64ToPtrInt64(d.YTSource.YTLookupTimeout),
			YTSelectTimeout:     pillars.MapOptionalInt64ToPtrInt64(d.YTSource.YTSelectTimeout),
			YTRetryCount:        pillars.MapOptionalInt64ToPtrInt64(d.YTSource.YTRetryCount),
		}
	}

	return dict, nil
}

func dictionariesFromModel(cryptoProvider crypto.Crypto, d []chmodels.Dictionary) ([]Dictionary, error) {
	var (
		result = make([]Dictionary, len(d))
		err    error
	)

	for i, dictionary := range d {
		result[i], err = dictionaryFromModel(cryptoProvider, dictionary)
		if err != nil {
			return nil, err
		}
	}

	return result, nil
}

type DictionaryRange struct {
	Min int64 `json:"min,omitempty"`
	Max int64 `json:"max,omitempty"`
}

func (dr *DictionaryRange) toModel() chmodels.DictionaryLifetimeRange {
	if dr == nil {
		return chmodels.DictionaryLifetimeRange{}
	}

	return chmodels.DictionaryLifetimeRange{
		Min:   dr.Min,
		Max:   dr.Max,
		Valid: true,
	}
}

type DictionaryLayout struct {
	Type        chmodels.DictionaryLayoutType `json:"type,omitempty"`
	SizeInCells *int64                        `json:"size_in_cells,omitempty"`
}

func (dl DictionaryLayout) toModel() chmodels.DictionaryLayout {
	return chmodels.DictionaryLayout{
		Type:        dl.Type,
		SizeInCells: pillars.MapPtrInt64ToOptionalInt64(dl.SizeInCells),
	}
}

type DictionaryStructure struct {
	ID         *DictionaryStructureID         `json:"id,omitempty"`
	Key        *DictionaryStructureKey        `json:"key,omitempty"`
	RangeMin   *DictionaryStructureAttribute  `json:"range_min,omitempty"`
	RangeMax   *DictionaryStructureAttribute  `json:"range_max,omitempty"`
	Attributes []DictionaryStructureAttribute `json:"attributes,omitempty"`
}

func (ds DictionaryStructure) toModel() chmodels.DictionaryStructure {
	return chmodels.DictionaryStructure{
		ID:         ds.ID.toModel(),
		Key:        ds.Key.toModel(),
		RangeMin:   ds.RangeMin.toModel(),
		RangeMax:   ds.RangeMax.toModel(),
		Attributes: structureAttributesToModel(ds.Attributes),
	}
}

func structureFromModel(s chmodels.DictionaryStructure) DictionaryStructure {
	res := DictionaryStructure{
		RangeMin:   structureAttributeFromModel(s.RangeMin),
		RangeMax:   structureAttributeFromModel(s.RangeMax),
		Attributes: structureAttributesFromModel(s.Attributes),
	}

	if s.ID.Valid {
		res.ID = &DictionaryStructureID{Name: s.ID.String}
	}

	if s.Key.Valid {
		res.Key = &DictionaryStructureKey{Attributes: structureAttributesFromModel(s.Key.Attributes)}
	}

	return res
}

type DictionaryStructureAttribute struct {
	Name         string  `json:"name,omitempty"`
	Type         string  `json:"type,omitempty"`
	NullValue    *string `json:"null_value,omitempty"`
	Expression   *string `json:"expression,omitempty"`
	Hierarchical *bool   `json:"hierarchical,omitempty"`
	Injective    *bool   `json:"injective,omitempty"`
}

func (dsa *DictionaryStructureAttribute) toModel() chmodels.DictionaryStructureAttribute {
	if dsa == nil {
		return chmodels.DictionaryStructureAttribute{}
	}

	return chmodels.DictionaryStructureAttribute{
		Name:         dsa.Name,
		Type:         dsa.Type,
		NullValue:    pillars.MapPtrStringToOptionalString(dsa.NullValue),
		Expression:   pillars.MapPtrStringToOptionalString(dsa.Expression),
		Hierarchical: pillars.MapPtrBoolToOptionalBool(dsa.Hierarchical),
		Injective:    pillars.MapPtrBoolToOptionalBool(dsa.Injective),
		Valid:        true,
	}
}

func structureAttributesToModel(a []DictionaryStructureAttribute) []chmodels.DictionaryStructureAttribute {
	res := make([]chmodels.DictionaryStructureAttribute, len(a))
	for i, attribute := range a {
		res[i] = attribute.toModel()
	}
	return res
}

func structureAttributeFromModel(a chmodels.DictionaryStructureAttribute) *DictionaryStructureAttribute {
	if !a.Valid {
		return nil
	}
	return &DictionaryStructureAttribute{
		Name:         a.Name,
		Type:         a.Type,
		NullValue:    pillars.MapOptionalStringToPtrString(a.NullValue),
		Expression:   pillars.MapOptionalStringToPtrString(a.Expression),
		Hierarchical: pillars.MapOptionalBoolToPtrBool(a.Hierarchical),
		Injective:    pillars.MapOptionalBoolToPtrBool(a.Injective),
	}
}

func structureAttributesFromModel(a []chmodels.DictionaryStructureAttribute) []DictionaryStructureAttribute {
	res := make([]DictionaryStructureAttribute, 0, len(a))
	for _, c := range a {
		res = append(res, *structureAttributeFromModel(c))
	}

	return res
}

type DictionaryStructureID struct {
	Name string `json:"name,omitempty"`
}

func (dsid *DictionaryStructureID) toModel() optional.String {
	if dsid == nil {
		return optional.String{}
	}

	return optional.NewString(dsid.Name)
}

type DictionaryStructureKey struct {
	Attributes []DictionaryStructureAttribute `json:"attributes,omitempty"`
}

func (dsk *DictionaryStructureKey) toModel() chmodels.DictionaryStructureKey {
	if dsk == nil {
		return chmodels.DictionaryStructureKey{}
	}

	return chmodels.DictionaryStructureKey{
		Attributes: structureAttributesToModel(dsk.Attributes),
		Valid:      true,
	}
}

type DictionarySourceHTTP struct {
	URL    string `json:"url,omitempty"`
	Format string `json:"format,omitempty"`
}

func (dsh *DictionarySourceHTTP) toModel() chmodels.DictionarySourceHTTP {
	if dsh == nil {
		return chmodels.DictionarySourceHTTP{}
	}

	return chmodels.DictionarySourceHTTP{
		URL:    dsh.URL,
		Format: dsh.Format,
		Valid:  true,
	}
}

type DictionarySourceMySQL struct {
	DB              string                         `json:"db,omitempty"`
	Table           string                         `json:"table,omitempty"`
	Port            *int64                         `json:"port,omitempty"`
	User            *string                        `json:"user,omitempty"`
	Password        *pillars.CryptoKey             `json:"password,omitempty"`
	Replicas        []DictionarySourceMySQLReplica `json:"replicas,omitempty"`
	Where           *string                        `json:"where,omitempty"`
	InvalidateQuery *string                        `json:"invalidate_query,omitempty"`
}

func (dsms *DictionarySourceMySQL) toModel() chmodels.DictionarySourceMySQL {
	if dsms == nil {
		return chmodels.DictionarySourceMySQL{}
	}

	return chmodels.DictionarySourceMySQL{
		DB:              dsms.DB,
		Table:           dsms.Table,
		Port:            pillars.MapPtrInt64ToOptionalInt64(dsms.Port),
		User:            pillars.MapPtrStringToOptionalString(dsms.User),
		Replicas:        mysqlReplicasToModel(dsms.Replicas),
		Where:           pillars.MapPtrStringToOptionalString(dsms.Where),
		InvalidateQuery: pillars.MapPtrStringToOptionalString(dsms.InvalidateQuery),
		Valid:           true,
	}
}

type DictionarySourceMySQLReplica struct {
	Host     string             `json:"host,omitempty"`
	Priority *int64             `json:"priority,omitempty"`
	Port     *int64             `json:"port,omitempty"`
	User     *string            `json:"user,omitempty"`
	Password *pillars.CryptoKey `json:"password,omitempty"`
}

func (dsmsr DictionarySourceMySQLReplica) toModel() chmodels.DictionarySourceMySQLReplica {
	return chmodels.DictionarySourceMySQLReplica{
		Host:     dsmsr.Host,
		Priority: pillars.MapPtrInt64ToOptionalInt64(dsmsr.Priority),
		Port:     pillars.MapPtrInt64ToOptionalInt64(dsmsr.Port),
		User:     pillars.MapPtrStringToOptionalString(dsmsr.User),
	}
}

func mysqlReplicasToModel(replicas []DictionarySourceMySQLReplica) []chmodels.DictionarySourceMySQLReplica {
	res := make([]chmodels.DictionarySourceMySQLReplica, len(replicas))
	for i, replica := range replicas {
		res[i] = replica.toModel()
	}
	return res
}

type DictionarySourceClickhouse struct {
	DB       string             `json:"db,omitempty"`
	Table    string             `json:"table,omitempty"`
	Host     string             `json:"host,omitempty"`
	Port     *int64             `json:"port,omitempty"`
	User     string             `json:"user,omitempty"`
	Password *pillars.CryptoKey `json:"password,omitempty"`
	Where    *string            `json:"where,omitempty"`
}

func (dsch *DictionarySourceClickhouse) toModel() chmodels.DictionarySourceClickhouse {
	if dsch == nil {
		return chmodels.DictionarySourceClickhouse{}
	}

	return chmodels.DictionarySourceClickhouse{
		DB:    dsch.DB,
		Table: dsch.Table,
		Host:  dsch.Host,
		Port:  pillars.MapPtrInt64ToOptionalInt64(dsch.Port),
		User:  dsch.User,
		Where: pillars.MapPtrStringToOptionalString(dsch.Where),
		Valid: true,
	}
}

type DictionarySourceMongoDB struct {
	DB         string             `json:"db,omitempty"`
	Collection string             `json:"collection,omitempty"`
	Host       string             `json:"host,omitempty"`
	Port       *int64             `json:"port,omitempty"`
	User       string             `json:"user,omitempty"`
	Password   *pillars.CryptoKey `json:"password,omitempty"`
}

func (dsm *DictionarySourceMongoDB) toModel() chmodels.DictionarySourceMongoDB {
	if dsm == nil {
		return chmodels.DictionarySourceMongoDB{}
	}

	return chmodels.DictionarySourceMongoDB{
		DB:         dsm.DB,
		Collection: dsm.Collection,
		Host:       dsm.Host,
		Port:       pillars.MapPtrInt64ToOptionalInt64(dsm.Port),
		User:       dsm.User,
		Valid:      true,
	}
}

type DictionarySourcePostgreSQL struct {
	DB              string             `json:"db,omitempty"`
	Table           string             `json:"table,omitempty"`
	Hosts           []string           `json:"hosts,omitempty"`
	Port            *int64             `json:"port,omitempty"`
	User            string             `json:"user,omitempty"`
	Password        *pillars.CryptoKey `json:"password,omitempty"`
	InvalidateQuery *string            `json:"invalidate_query,omitempty"`
	SSLMode         *string            `json:"ssl_mode,omitempty"`
}

func (dspg *DictionarySourcePostgreSQL) toModel() chmodels.DictionarySourcePostgreSQL {
	if dspg == nil {
		return chmodels.DictionarySourcePostgreSQL{}
	}

	return chmodels.DictionarySourcePostgreSQL{
		DB:              dspg.DB,
		Table:           dspg.Table,
		Hosts:           dspg.Hosts,
		Port:            pillars.MapPtrInt64ToOptionalInt64(dspg.Port),
		User:            dspg.User,
		InvalidateQuery: pillars.MapPtrStringToOptionalString(dspg.InvalidateQuery),
		SSLMode:         chmodels.PostgresSSLMode(pillars.MapPtrStringToOptionalString(dspg.SSLMode)),
		Valid:           true,
	}
}

type DictionarySourceYT struct {
	Clusters            []string           `json:"clusters,omitempty"`
	Table               string             `json:"table,omitempty"`
	Keys                []string           `json:"keys,omitempty"`
	Fields              []string           `json:"fields,omitempty"`
	DateFields          []string           `json:"date_fields,omitempty"`
	DatetimeFields      []string           `json:"datetime_fields,omitempty"`
	Query               *string            `json:"query,omitempty"`
	User                *string            `json:"user,omitempty"`
	Token               *pillars.CryptoKey `json:"token,omitempty"`
	ClusterSelection    *string            `json:"cluster_selection,omitempty"`
	UseQueryForCache    *bool              `json:"use_query_for_cache,omitempty"`
	ForceReadTable      *bool              `json:"force_read_table,omitempty"`
	RangeExpansionLimit *int64             `json:"range_expansion_limit,omitempty"`
	InputRowLimit       *int64             `json:"input_row_limit,omitempty"`
	OutputRowLimit      *int64             `json:"output_row_limit,omitempty"`
	YTSocketTimeout     *int64             `json:"yt_socket_timeout_msec,omitempty"`
	YTConnectionTimeout *int64             `json:"yt_connection_timeout_msec,omitempty"`
	YTLookupTimeout     *int64             `json:"yt_lookup_timeout_msec,omitempty"`
	YTSelectTimeout     *int64             `json:"yt_select_timeout_msec,omitempty"`
	YTRetryCount        *int64             `json:"yt_retry_count,omitempty"`
}

func (source *DictionarySourceYT) toModel() chmodels.DictionarySourceYT {
	if source == nil {
		return chmodels.DictionarySourceYT{}
	}

	return chmodels.DictionarySourceYT{
		Clusters:            source.Clusters,
		Table:               source.Table,
		Keys:                source.Keys,
		Fields:              source.Fields,
		DateFields:          source.DateFields,
		DatetimeFields:      source.DatetimeFields,
		Query:               pillars.MapPtrStringToOptionalString(source.Query),
		User:                pillars.MapPtrStringToOptionalString(source.User),
		ClusterSelection:    chmodels.YTClusterSelection(pillars.MapPtrStringToOptionalString(source.ClusterSelection)),
		UseQueryForCache:    pillars.MapPtrBoolToOptionalBool(source.UseQueryForCache),
		ForceReadTable:      pillars.MapPtrBoolToOptionalBool(source.ForceReadTable),
		RangeExpansionLimit: pillars.MapPtrInt64ToOptionalInt64(source.RangeExpansionLimit),
		InputRowLimit:       pillars.MapPtrInt64ToOptionalInt64(source.InputRowLimit),
		OutputRowLimit:      pillars.MapPtrInt64ToOptionalInt64(source.OutputRowLimit),
		YTSocketTimeout:     pillars.MapPtrInt64ToOptionalInt64(source.YTSocketTimeout),
		YTConnectionTimeout: pillars.MapPtrInt64ToOptionalInt64(source.YTConnectionTimeout),
		YTLookupTimeout:     pillars.MapPtrInt64ToOptionalInt64(source.YTLookupTimeout),
		YTSelectTimeout:     pillars.MapPtrInt64ToOptionalInt64(source.YTSelectTimeout),
		YTRetryCount:        pillars.MapPtrInt64ToOptionalInt64(source.YTRetryCount),
		Valid:               true,
	}
}

func (chcfg *ClickHouseConfig) AddDictionary(cryptoProvider crypto.Crypto, dict chmodels.Dictionary) error {
	for _, d := range chcfg.Dictionaries {
		if d.Name == dict.Name {
			return semerr.AlreadyExistsf("dictionary %q already exists", dict.Name)
		}
	}

	d, err := dictionaryFromModel(cryptoProvider, dict)
	if err != nil {
		return err
	}

	chcfg.Dictionaries = append(chcfg.Dictionaries, d)
	return nil
}

func (chcfg *ClickHouseConfig) UpdateDictionary(cryptoProvider crypto.Crypto, dict chmodels.Dictionary) error {
	for i, d := range chcfg.Dictionaries {
		if d.Name == dict.Name {
			d, err := dictionaryFromModel(cryptoProvider, dict)
			if err != nil {
				return err
			}

			chcfg.Dictionaries[i] = d
			return nil
		}
	}

	return semerr.NotFoundf("dictionary %q not found", dict.Name)
}

func (chcfg *ClickHouseConfig) DeleteDictionary(name string) error {
	for i, d := range chcfg.Dictionaries {
		if d.Name == name {
			chcfg.Dictionaries = append(chcfg.Dictionaries[:i], chcfg.Dictionaries[i+1:]...)
			return nil
		}
	}

	return semerr.NotFoundf("dictionary %q not found", name)
}
