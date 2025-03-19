-- get md5 hash for any object
$get_md5 = ($obj) -> (Digest::Md5Hex(StablePickle($obj)));
$get_md5_without_pickle = ($obj) -> (Digest::Md5Hex($obj));

-- converts
$to_str = ($container) -> (cast($container AS String));

-- parse Yson-node to JSON
$get_json_from_yson = ($yson) -> (Yson::ParseJson(Yson::ConvertToString($yson)));

-- yson options
$autoconvert_options = Yson::Options(true AS AutoConvert);
$strict_options = Yson::Options(false AS Strict);

-- convert yson
$yson_to_str = ($container)  -> (Yson::ConvertToString($container, $autoconvert_options));
$yson_to_int64 = ($container)  -> (Yson::ConvertToInt64($container, $autoconvert_options));
$yson_to_uint64 = ($container)  -> (Yson::ConvertToUint64($container, $autoconvert_options));
$yson_to_list = ($container) -> (Yson::ConvertToList($container, $autoconvert_options));
$convert_to_optional = ($container) -> (ListMap($container, ($x) -> {return Just($x)},));

-- try to lookup value with default value
$lookup = ($container, $key) -> (Yson::Lookup($container, $key, Yson::Options(false AS Strict)));
$lookup_string = ($container, $key, $default) -> (NVL(Yson::LookupString($container, $key, $autoconvert_options), $default));
$lookup_bool = ($container, $key, $default) -> (NVL(Yson::LookupBool($container, $key, $autoconvert_options), $default));
$lookup_string_list = ($container, $key) -> (Yson::ConvertToStringList($lookup($container, $key), $autoconvert_options));
$lookup_int64 = ($container, $key, $default) -> (NVL(Yson::LookupInt64($container, $key, $autoconvert_options), $default));



EXPORT $get_md5, $get_md5_without_pickle,
    $to_str,
    $get_json_from_yson,
    $autoconvert_options, $strict_options,
    $yson_to_str,$yson_to_list,$yson_to_uint64, $convert_to_optional,
    $lookup_string, $lookup_bool, $lookup_int64,
    $lookup_string_list, $lookup
    ;
