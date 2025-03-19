from collections import namedtuple

Record = namedtuple(
    "Record",
    [
        'geonameid',
        'name',
        'asciiname',
        'alternatenames',
        'latitude',
        'longitude',
        'feature_class',
        'feature_code',
        'country_code',
        'cc2',
        'admin1_code',
        'admin2_code',
        'admin3_code',
        'admin4_code',
        'population',
        'elevation',
        'dem',
        'timezone',
        'modification_date',
    ],
)


def parse_record(s):
    fields = s.rstrip().split("\t")
    return Record(*fields[0:14], int(fields[14]), *fields[15:])
