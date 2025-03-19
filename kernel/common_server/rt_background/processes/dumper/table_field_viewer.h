#pragma once

#include "meta_parser.h"

#include <kernel/common_server/rt_background/processes/common/yt.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {

    using TSchema = TYtProcessTraits::TSchema;

    class ITableFieldViewer {
    public:
        ITableFieldViewer(const IBaseServer& server);

        virtual IDumperMetaParser::TPtr Construct(const TString& key) const = 0;
        virtual NYT::TTableSchema GetYtSchema() const = 0;

        virtual ~ITableFieldViewer() = default;

        const IBaseServer& GetServer() const { return Server; }

    protected:
        const IBaseServer& Server;
    };

    class TDBTableFieldViewer: public ITableFieldViewer {
    public:
        TDBTableFieldViewer(const TString& tableName, const TString& dBName, const TString& dBType,
                            bool dbDefineSchema, const IBaseServer& server);

        virtual IDumperMetaParser::TPtr Construct(const TString& key) const override;

        virtual NYT::TTableSchema GetYtSchema() const override;

    private:
        TSchema GetNativeYTSchema() const;

    private:
        CSA_DEFAULT(TDBTableFieldViewer, TString, TableName);
        CSA_DEFAULT(TDBTableFieldViewer, TString, DBName);
        CSA_DEFAULT(TDBTableFieldViewer, TString, DBType);
        CS_ACCESS(TDBTableFieldViewer, bool, DBDefineSchema, false);
    };

} // namespace NCS
