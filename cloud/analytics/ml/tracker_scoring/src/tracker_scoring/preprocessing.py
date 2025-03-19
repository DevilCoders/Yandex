import re
import pyspark.sql.functions as F
import pyspark.sql.types as T


def add_brackets(col):
    replace_coma = F.regexp_replace(col, r'(,\s?)', '","')
    return F.concat(F.lit('["'), replace_coma, F.lit('"]'))


def company_size(size):
    decoder = {
        'm': '1-100',
        'l': '101-1000',
        'xl': '1000+'
    }
    return "Указан размер организации " + decoder[size]

company_size_udf = F.udf(company_size, returnType=T.StringType())


def phone_decorator(phone):
    if isinstance(phone, str):
        return re.sub("[^0-9]", "", phone)
    else:
        return '-'

phone_decorator_udf = F.udf(phone_decorator, returnType=T.StringType())
