Converts a stdin stream of NKiwiWorm::TRecord (received by kwworm subscribe, for
exmaple) to a stdout stream of NKiwiWorm::TCalcParams.

Arguments: optional. A space-separated list of parameter names that must exist 
in every output record. If there is no matching attribute in an input record, a 
null-valued parameter (IsNull(@Param) == true) will be written instead.

