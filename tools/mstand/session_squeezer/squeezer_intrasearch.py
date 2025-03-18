# coding=utf-8
import session_squeezer.squeezer_common as sq_common

INTRA_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "browser", "type": "string"},
    {"name": "bucket", "type": "int64"},
    {"name": "clid", "type": "string"},
    {"name": "correctedquery", "type": "string"},
    {"name": "dir_show", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "dwelltime_on_service", "type": "int64"},
    {"name": "fraud_bits", "type": "int64"},
    {"name": "hasmisspell", "type": "boolean"},
    {"name": "is_match", "type": "boolean"},
    {"name": "maxrelevpredict", "type": "double"},
    {"name": "minrelevpredict", "type": "double"},
    {"name": "page", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "pos", "type": "int64"},
    {"name": "query", "type": "string"},
    {"name": "queryregion", "type": "int64"},
    {"name": "referer", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "restype", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "sources", "type": "any"},
    {"name": "suggest", "type": "any"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "userpersonalization", "type": "boolean"},
    {"name": "userregion", "type": "int64"},
    {"name": "vars", "type": "string"},
    {"name": "visual_pos", "type": "int64"},
    {"name": "wiz_show", "type": "int64"},
    {"name": "wizard", "type": "boolean"},
    {"name": "suggestion", "type": "string"},
    {"name": "target", "type": "string"},
    {"name": "saas_service", "type": "string"},
    {"name": "processed_query", "type": "string"},
]

INTRA_DESKTOP_SAMPLE = u"""{"action_index":0,"browser":"Unknown","bucket":null,"clid":"","correctedquery":"",
"dir_show":0,"domregion":null,"dwelltime_on_service":null,"fraud_bits":null,"hasmisspell":false,"is_match":true,
"maxrelevpredict":null,"minrelevpredict":null,"page":null,"path":null,"pos":null,"query":"трансляции",
"queryregion":null,"referer":"other","reqid":"1527575499429573-11017834492307263877-sas1-2766-SAAS","restype":null,
"servicetype":"intrasearch","sources":[],"suggest":null,"suggestion":null,"target":null,"testid":"0","ts":1527575499,
"type":"request","ui":null,"url":null,"userpersonalization":false,"userregion":null,"vars":null,"visual_pos":null,
"wiz_show":0,"wizard":false,"yuid":"is1120000000000048"}"""


class ActionsSqueezerIntra(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add saas_service and processed_query
    """
    VERSION = 2

    YT_SCHEMA = INTRA_YT_SCHEMA
    SAMPLE = INTRA_DESKTOP_SAMPLE

    def __init__(self):
        super(ActionsSqueezerIntra, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if self.check_request(request):
                exp_bucket = self.check_experiments(args, request)
                collect_intra_actions(args.result_actions, exp_bucket, request)

    @staticmethod
    def check_request(request):
        return request.IsA("TSaasRequest") and (
            request.SaasService.startswith("intrasearch") or
            request.SaasService == "startrek"
        )


def collect_intra_actions(known_actions, exp_bucket, request):
    base_data = sq_common.prepare_base_data(exp_bucket, request)
    request_data = sq_common.build_request_data(base_data, request)
    request_data["saas_service"] = request.SaasService
    request_data["processed_query"] = request.ProcessedQuery
    known_actions.append(request_data)
    sq_common.append_clicks(known_actions, base_data, request)
