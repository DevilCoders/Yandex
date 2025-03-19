#include "service_actor.h"

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/stream/str.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

void DumpFsLink(IOutputStream& out, const ui64 tabletId, const TString& fsId)
{
    out << "<a href='../tablets?TabletID=" << tabletId << "'>"
        << fsId << "</a>";
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleHttpInfo(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& request = ev->Get()->Request;
    TString uri{request.GetUri()};
    LOG_DEBUG(ctx, TFileStoreComponents::SERVICE,
        "HTTP request: %s", uri.c_str());

    TStringStream out;
    if (State) {
        HTML(out) {
            H3() { out << "Sessions"; }

            TABLE_SORTABLE_CLASS("table table-bordered") {
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() { out << "ClientId"; }
                        TABLEH() { out << "FileSystemId"; }
                        TABLEH() { out << "SessionId"; }
                    }
                }

                State->VisitSessions([&] (const TSessionInfo& session) {
                    TABLER() {
                        TABLED() { out << session.ClientId; }
                        TABLED() {
                            DumpFsLink(
                                out,
                                session.TabletId,
                                session.FileSystemId
                            );
                        }
                        TABLED() { out << session.SessionId; }
                    }
                });
            }
        }
    } else {
        out << "State not ready yet" << Endl;
    }

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvHttpInfoRes>(out.Str()));
}

}   // namespace NCloud::NFileStore::NStorage
