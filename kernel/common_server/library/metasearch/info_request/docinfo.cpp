#include <search/meta/context.h>
#include <search/meta/doc.h>
#include <search/meta/info.h>
#include <search/meta/metasearch.h>

#include <library/cpp/json/json_value.h>

#include <util/system/hostname.h>

namespace {
    class TMetaDocInfoCalculator : public IMetaInfoCalculator {
    private:
        class TTask : public IMetaInfoTask {
        public:
            TTask(TClientInfo* ci, const TDocHandle handle)
                : IMetaInfoTask(ci)
                , Handle(handle)
            {
            }

            inline const NJson::TJsonValue& GetInfo() const {
                return Info;
            }
            inline bool HasInfo() const {
                return Info.GetType() == NJson::JSON_ARRAY;
            }

            virtual void FillRequestData(NScatter::TRequestData& rd) const override {
                rd.Url = TString("info=docinfo:docid:") + Handle.ToString();
            }
            virtual NScatter::TParseResult DoParse(const NScatter::TTaskReply& reply) override {
                NJson::ReadJsonFastTree(reply.Data, &Info);
                return { Finished };
            }

        private:
            const TDocHandle Handle;
            NJson::TJsonValue Info;
        };

    public:
        virtual void Execute(
            const TMetaSearch& owner,
            TMetaSearchContext& ctx,
            const TCgiParameters& cgiParam,
            const TRequestParams& params,
            IOutputStream& infoBuf
        ) override {
            Y_UNUSED(cgiParam);

            TStringBuf docIdStr;
            if(!params.InfoParams.Params.TryGet("docid", docIdStr)) {
                infoBuf << "cannot find DocId";
                return;
            }

            TDocHandle docId(docIdStr);
            TVector<TIntrusivePtr<TTask>> tasks;
            if (!docId.DocRoute) {
                for (size_t i = 0; i < owner.ClientCount(); ++i) {
                    TClientInfo &ci = ctx.ClientsInfo[i];
                    tasks.push_back(new TTask(&ci, docId.ClientDocId()));
                }
            } else {
                if (docId.ClientNum() >= owner.ClientCount()) {
                    infoBuf << "incorrect ClientNum: " << docId.ClientNum();
                    return;
                }

                TClientInfo &ci = ctx.ClientsInfo[docId.ClientNum()];
                tasks.push_back(new TTask(&ci, docId.ClientDocId()));
            }

            NScatter::TTaskList tasks_ = { tasks.begin(), tasks.end() };
            ctx.RunTasks("docinfo", tasks_);

            NJson::TJsonValue result(NJson::JSON_ARRAY);
            for (auto&& task : tasks) {
                if (!task->HasInfo()) {
                    continue;
                }

                const TString& source = HostName()
                    + ":" + ToString(task->Source()->ClientNum())
                    + ":" + task->Source()->SearchSource()->Descr
                    + ":" + task->Source()->SearchSource()->Group;
                for (auto value : task->GetInfo().GetArray()) {
                    value["Sources"].AppendValue(source);
                    result.AppendValue(value);
                }
            }
            infoBuf << result.GetStringRobust();
        }
    };
}

IMetaInfoCalculator::TFactory::TRegistrator<TMetaDocInfoCalculator> MetaDocInfoCalculatorRegistrator("docinfo");
