import session_squeezer.squeezer_common as sq_common

RECOMMENDER_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "dwelltime_on_service", "type": "int64"},
    {"name": "fraud_bits", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "page", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "pos", "type": "int64"},
    {"name": "query", "type": "string"},
    {"name": "queryregion", "type": "int64"},
    {"name": "reqid", "type": "string"},
    {"name": "restype", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "userregion", "type": "int64"},
    {"name": "vars", "type": "string"},
    {"name": "visual_pos", "type": "int64"},
    {"name": "target", "type": "string"},
    {"name": "blocks", "type": "any"},
    {"name": "request_source", "type": "string"},
    {"name": "main_reqid", "type": "string"},
    {"name": "connected_pos", "type": "int64"},
    {"name": "connected_url", "type": "string"},
]


class ActionsSqueezerRecommender(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add request_source
    3: main_reqid + connected_pos + connected_url
    """
    VERSION = 3

    YT_SCHEMA = RECOMMENDER_YT_SCHEMA

    def __init__(self):
        super(ActionsSqueezerRecommender, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if request.IsA("TRecommendationRequestProperties"):
                exp_bucket = self.check_experiments(args, request)
                collect_recommender_actions(args.result_actions, exp_bucket, request)


def collect_recommender_actions(known_actions, exp_bucket, request):
    base_data = sq_common.prepare_base_data(exp_bucket, request, add_ui=True)
    connected_result = request.GetConnectedResult()
    if connected_result:
        connected_request = connected_result.GetParentRequest()
        if connected_request:
            base_data["main_reqid"] = connected_request.ReqId
        base_data["connected_pos"] = connected_result.Position
        connected_main_result = connected_result.GetMainResult()
        base_data["connected_url"] = getattr(connected_main_result, "Url", None)

    request_data = build_recommender_request(base_data, request)
    known_actions.append(request_data)
    sq_common.append_clicks(known_actions, base_data, request)


def build_recommender_request(base_data, request):
    blocks = list(get_blocks(request))
    request_data = dict(
        base_data,
        ts=request.Timestamp,
        type="request",
        query=request.Query,
        blocks=blocks,
        request_source=request.RequestSource,
    )
    return request_data


def get_blocks(request):
    """
    :type request: libra.TRequest
    :rtype: __generator[dict]
    """
    for description in sq_common.get_all_blocks(request):
        if description.restype == "recommendation":
            yield description.build_block_data()
