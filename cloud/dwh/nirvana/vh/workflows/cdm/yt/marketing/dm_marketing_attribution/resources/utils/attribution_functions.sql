$script = FileContent("attribution_functions.py");

$apply_attribution = Python3::apply_attribution(
    Callable<(List<Tuple<String, String, Uint64, String>>,String, String)->List<Tuple<Uint64, String, Double>>>,
    $script
);

$apply_attribution_all = Python3::apply_attribution_all(
    Callable<(List<Tuple<String, String, Uint64, String, String, String, String, String, String, String, String, String,  String, String, String, String, String>>,String)->List<Tuple<String, String, Uint64, String, String, String, String, String, String, String, String, String, String, String, String, String, String,  List<Double>>>>,
    $script
);

EXPORT $apply_attribution, $apply_attribution_all;
