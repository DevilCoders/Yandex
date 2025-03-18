#include "squeezer_web_surveys.h"

#include <quality/ab_testing/lib_calc_session_metrics/common/words.h> // NSessionsMetrics::GetQuery
#include <tools/mstand/squeeze_lib/requests/common/base_request.h> // NMstand::GetSerpType

namespace NMstand {

using namespace NYT;

/////////////////////////////// TWebSurveysActionsSqueezer
TTableSchema TWebSurveysActionsSqueezer::GetSchema() const {
    TTableSchema schema;
    schema.AddColumn(TColumnSchema().Name("yuid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ts").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("action_index").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("bucket").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_match").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("testid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("servicetype").Type(NYT::VT_STRING));

    schema.AddColumn(TColumnSchema().Name("answer_id").Type(NYT::VT_INT32));
    schema.AddColumn(TColumnSchema().Name("answers").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("block_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("block_name").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("browser").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("comment_text").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("event_id").Type(NYT::VT_STRING));
    schema.AddColumn("multi_answers", NTi::Optional(NTi::List(NTi::Tuple({{NTi::Int32()}, {NTi::List(NTi::Int32())}}))));
    schema.AddColumn(TColumnSchema().Name("query_text").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("question_id").Type(NYT::VT_INT32));
    schema.AddColumn(TColumnSchema().Name("questions").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("reqid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("result_block_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("selected_option").Type(NYT::VT_INT32));
    schema.AddColumn(TColumnSchema().Name("survey_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("survey_type").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("type").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ui").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("url").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("visibility").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("wizard_name").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("prism_segment").Type(NYT::VT_UINT64));
    schema.AddColumn(TColumnSchema().Name("dwelltime").Type(NYT::VT_UINT64));
    schema.AddColumn(TColumnSchema().Name("wizard_position").Type(NYT::VT_STRING));
    
    return schema.Strict(true);
}

ui32 TWebSurveysActionsSqueezer::GetVersion() const
{
    return 1;
}

bool TWebSurveysActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TTouchYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TPadYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TMobileAppYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TYandexWebRequest*>(request) != nullptr;
}

// In order to convert NBaobab::TAttr -> NYT::TNode
namespace {
    NYT::TNode SerializeToNode(const NBaobab::TAttrsMap&);
    NYT::TNode SerializeToNode(const NBaobab::TAttrsArray&);

    NYT::TNode SerializeToNode(const NBaobab::TAttr& attr) {
        switch (attr.GetType()) {
            case NBaobab::EAttrType::Double:
                return attr.Get<double>();
            case NBaobab::EAttrType::Int:
                return attr.Get<i64>();
            case NBaobab::EAttrType::UnsignedInt:
                return attr.Get<ui64>();
            case NBaobab::EAttrType::String:
                return attr.Get<TString>();
            case NBaobab::EAttrType::Bool:
                return attr.Get<bool>();
            case NBaobab::EAttrType::Array:
                return SerializeToNode(attr.Get<NBaobab::TAttrsArray>());
            case NBaobab::EAttrType::Dict:
                return SerializeToNode(attr.Get<NBaobab::TAttrsMap>());
            default:
                return NYT::TNode::CreateEntity();
        }
    }

    NYT::TNode SerializeToNode(const NBaobab::TAttrsMap& attrsMap) {
        auto res = NYT::TNode::CreateMap();
        for (const auto& [k,v] : attrsMap) {
            res[k] = SerializeToNode(v);
        }
        return res;
    }

    NYT::TNode SerializeToNode(const NBaobab::TAttrsArray& attrsArray) {
        auto res = NYT::TNode::CreateList();
        for (const auto& el: attrsArray) {
            auto val = SerializeToNode(el);
            res.Add(val);
        }
        return res;
    }
}

namespace {
    TMaybe<i64> GetAnswerId(const NBaobab::TBlock& answerBlock) {
        if (const auto& id = NBaobab::GetAttrValueUnsafe<i64>(answerBlock, "answer_id")) {
            return id.GetRef();
        }
        if (const auto& id = NBaobab::GetAttrValueUnsafe<i64>(answerBlock, "id")) {
            return id.GetRef();
        }
        return Nothing();
    }

    NYT::TNode GetAnswers(const NBaobab::TBlock& surveyBlock) {
        NYT::TNode::TListType answers;
        for (const auto& answerBlock : NBaobab::TBFSer(surveyBlock,  NBaobab::NameFilter("answer"))) {
            if (const auto& answerId = GetAnswerId(answerBlock); answerId.Defined()) {
                answers.push_back(answerId.GetRef());
            }
        }
        return NYT::TNode::CreateList(answers);
    }

    NYT::TNode GetMultiAnswers(const NBaobab::TBlock& surveyBlock) {
        THashMap<i32, NYT::TNode::TListType> answers;
        for (const auto& answerBlock : NBaobab::TBFSer(surveyBlock,  NBaobab::NameFilter("answer"))) {
            if (const auto& idQuestion = NBaobab::GetAttrValueUnsafe<i64>(answerBlock, "idQuestion")) {
                if (const auto& answerId = GetAnswerId(answerBlock); answerId.Defined()) {
                    answers[idQuestion.GetRef()].push_back(answerId.GetRef());
                }
            }
        }
        NYT::TNode::TListType result;
        for (const auto& [k, v] : answers) {
            result.push_back(
                NYT::TNode::CreateList({NYT::TNode(k), NYT::TNode::CreateList(v)})
            );
        }
        return NYT::TNode::CreateList(result);
    }

    TString GetSurveyType(const NBaobab::TBlock& surveyBlock) {
        for (const auto& answerBlock : NBaobab::TBFSer(surveyBlock,  NBaobab::NameFilter("answer"))) {
            if (answerBlock.GetAttrs().has("text")) {
                return "text";
            }
            if (answerBlock.GetAttrs().has("emoji")) {
                return "emoji";
            }
        }
        return "unknown";
    }

    TMaybe<bool> CheckBlockVisible(
        const NRA::TRequest* request,
        const NBaobab::TRequestJoiner& joiner,
        const NBaobab::TBlock& block
    ) {
        for (const auto& event : joiner.GetEventsByBlockSubtree(block)) {
            if (const auto click = dynamic_cast<const NBaobab::IClick*>(event.Get())) {
                return true;
            }
        }

        NMouseTrack::TMouseTrackRequestData mouseTrackRequestData = NMouseTrack::ExtractFromRequest(*request);
        for (const auto& mouseTrackBlock : mouseTrackRequestData.BlocksData) {
            if (mouseTrackBlock.Get()->HasBlockCounterId() && block.GetID() == mouseTrackBlock.Get()->GetBlockCounterId()) {
                auto blockData = mouseTrackBlock.Get();
                if (blockData && blockData->HasTop()) {
                    double mouseTrackTop = blockData->GetTop();
                    if (mouseTrackTop > 0 && mouseTrackTop < 20000) {
                        return mouseTrackRequestData.MaxScrollYBottom > mouseTrackTop;
                    }
                }
            }
        }
        return Nothing();
    }

    TMaybe<int64_t> GetPrismSegment(const NRA::TRequest* request) {
        const auto* misc = dynamic_cast<const NRA::TMiscRequestProperties*>(request);
        if (misc) {
            const NRA::TNameValueMap& searchPropsUpper = misc->GetSearchPropsValues();
            if (const auto* valuePtr = searchPropsUpper.FindPtr("UPPER.PrismBigBLog.prism_segment")) {
                int64_t value;
                if (TryFromString(*valuePtr, value)) {
                    return value;
                }                
            }
        }
        return Nothing();
    }

    NYT::TNode GetRequestData(
        const NRA::TRequest* request,
        const NBaobab::TRequestJoiner& joiner,
        const NBaobab::TBlock& surveyBlock
    ) {
        auto requestData = NYT::TNode::CreateMap();
        requestData["yuid"] = request->GetUID();
        requestData["ts"] = request->GetTimestamp();
        requestData["reqid"] = request->GetReqID();
        requestData["type"] = "request";
        requestData["browser"] = request->GetBrowser();
        requestData["ui"] = ToString(GetSerpType(*request));
        if (const auto& queryText = NSessionsMetrics::GetQuery(request)) {
            requestData["query_text"] = queryText.GetRef();
        }

        if (const auto& resultBlock = NBaobab::GetAncestorResultBlock(surveyBlock)) {
            requestData["result_block_id"] = resultBlock->GetID();
            if (const auto& documentUrl = NBaobab::GetAttrValueUnsafe<TString>(*resultBlock, "documentUrl")) {
                requestData["url"] = documentUrl.GetRef();
            }
            if (const auto& wizardName = NBaobab::GetAttrValueUnsafe<TString>(*resultBlock, "wizard_name")) {
                requestData["wizard_name"] = wizardName.GetRef();
            }
            if (const auto& visible = CheckBlockVisible(request, joiner, resultBlock.GetRef())) {
                requestData["visibility"] = visible.GetRef();
            }
            if (const auto& wizardPos = NBaobab::GetAttrValueUnsafe<TString>(*resultBlock, "pos")) {
                requestData["wizard_position"] = wizardPos.GetRef();
            }
        }
        if (const auto& surveyId = NBaobab::GetAttrValueUnsafe<TString>(surveyBlock, "surveyId")) {
            requestData["survey_id"] = surveyId.GetRef();
        }
        requestData["answers"] = GetAnswers(surveyBlock);
        requestData["multi_answers"] = GetMultiAnswers(surveyBlock);
        requestData["survey_type"] = GetSurveyType(surveyBlock);
        if (const auto& questions = NBaobab::GetAttrValueUnsafe<NBaobab::TAttrsArray>(surveyBlock, "questions")) {
            requestData["questions"] = SerializeToNode(questions.GetRef());
        }
        if (const auto& prismSegment = GetPrismSegment(request)){
            requestData["prism_segment"] = prismSegment.GetRef();
        }
        requestData["dwelltime"] = request->GetDwellTime();

        return requestData;
    }

    NYT::TNode::TListType GetEventsData(
        const NYT::TNode& requestData,
        const NBaobab::TRequestJoiner& joiner,
        const NBaobab::TBlock& surveyBlock
    ) {
        NYT::TNode::TListType result;
        for (const auto& event : joiner.GetEventsByBlockSubtree(surveyBlock)) {
            auto eventData = requestData;
            eventData["ts"] = event->GetContext().GetServerTime();
            if (const auto clientEvent = dynamic_cast<const NBaobab::IClientEvent*>(event.Get())) {
                if (clientEvent->GetEventID().Defined()) {
                    eventData["event_id"] = clientEvent->GetEventID().GetRef();
                }
                eventData["block_id"] = clientEvent->GetBlockID();
            }

            if (const auto click = dynamic_cast<const NBaobab::IClick*>(event.Get())) {
                eventData["type"] = "click";

                if (auto clickBlock = NBaobab::GetBlockByID(joiner, click->GetBlockID())) {
                    eventData["block_name"] = clickBlock->GetName();
                    if (const auto& answerId = GetAnswerId(clickBlock.GetRef())) {
                        eventData["answer_id"] = answerId.GetRef();
                    }
                    if (const auto& idQuestion = NBaobab::GetAttrValueUnsafe<i64>(clickBlock.GetRef(), "idQuestion")) {
                        eventData["question_id"] = idQuestion.GetRef();
                    }
                }
                result.push_back(eventData);
            }
            else if (const auto tech = dynamic_cast<const NBaobab::ITech*>(event.Get())) {
                eventData["type"] = "techevent";

                if (const auto& commentText = NBaobab::GetAttrValueUnsafe<TString>(tech->GetData(), "commentText")) {
                    eventData["comment_text"] = commentText.GetRef();
                }
                if (const auto& selectedOption = NBaobab::GetAttrValueUnsafe<i64>(tech->GetData(), "selectedOption")) {
                    eventData["selected_option"] = selectedOption.GetRef();
                }
                if (const auto& commentId = NBaobab::GetAttrValueUnsafe<i64>(tech->GetData(), "commentId")) {
                    eventData["question_id"] = commentId.GetRef();
                }
                result.push_back(eventData);
            }
        }
        std::sort(result.begin(), result.end(), [](const NYT::TNode& e1, const NYT::TNode& e2) {
            return e1["ts"].AsInt64() < e2["ts"].AsInt64();
        });
        return result;
    }
}

void TWebSurveysActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        if (!CheckRequest(request)) {
            continue;
        }
        try {
            if (auto baobabProps = dynamic_cast<const NRA::TBaobabProperties*>(request)) {
                const auto& joiner = baobabProps->GetRequestJoiner();
                const TMaybe<NBaobab::TBlock> root = joiner.GetRoot();
                if (root) {
                    auto bucketInfo = CheckExperiments(args, request);
                    for (const auto& surveyBlock : NBaobab::TBFSer(*root, NBaobab::NameFilter("user-survey"))) {
                        auto requestData = GetRequestData(request, joiner, surveyBlock);
                        args.ResultActions.emplace_back(requestData, bucketInfo);
                        for (const auto& eventData : GetEventsData(requestData, joiner, surveyBlock)) {
                            args.ResultActions.emplace_back(eventData, bucketInfo);
                        }
                    }
                }
            }
        } catch (...) {
            Cerr << "Fail a request process: reqid: " << request->GetReqID() << Endl;
        }
    }
}
/////////////////////////////// TWebSurveysActionsSqueezer End

}; // NMstand
