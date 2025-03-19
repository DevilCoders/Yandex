#include "auto_teamlead.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/system/file.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/thread/factory.h>

namespace NCoolBot {
namespace {

////////////////////////////////////////////////////////////////////////////////

const TStringBuf HELP_MESSAGE =
    "/eval_ticket $ticket $score [$user]- set personal score for ticket $ticket"
    " [and user $user] to value $score, $score should be one of 1, 2, 4, 8, 16,"
    " 32, 64\n"
    "/list_tickets - list your tickets and their scores\n"
    "/start_ticket_evaluation - switch on ticket evaluation mode for user"
    " and show the next unevaluated ticket, wait for the score, then show the"
    " next ticket and so on\n"
    "/stop_ticket_evaluation - exit ticket evaluation mode\n"
    "/start_ticket_discussion - enter ticket discussion mode\n"
    "/stop_ticket_discussion - exit ticket discussion mode\n"
    "/make_plan - redistribute tickets\n"
    "/whoami - show chat_id\n"
;

const TStringBuf TOUGH_CHOICE_STICKER = "CAACAgIAAxkBAAIZc2BJC9h3BZB8Ws-AJcVF3NrAklsLAAL1CAACgJYPAAF4N-wMHSiu9x4E";
const TStringBuf REST_STICKER = "CAACAgIAAxkBAAIZhmBJD_LRJfXYrmHrCIjSYW_-8DXkAALoCAACgJYPAAGAGfm2T0QYjR4E";
const TStringBuf START_STICKER = "CAACAgIAAxkBAAIeCWBLbXlR2kmrAmGR7OoyHdVzxUGZAAI3AANQV40QezHeNAkBs4geBA";

const TDuration REEVALUATION_INTERVAL = TDuration::Days(1);
const TDuration CLOSE_DEADLINE_INTERVAL = TDuration::Days(2);
const TDuration DISCUSSION_INTERVAL = TDuration::Days(3);
const TDuration SCORE_UNIT = TDuration::Hours(12);

////////////////////////////////////////////////////////////////////////////////

enum class ECommand
{
    Unknown = 0,
    EvalTicket = 1,
    ListMyTickets = 2,
    StartTicketEvaluation = 3,
    StopTicketEvaluation = 4,
    MakePlan = 5,
    Start = 6,
    Help = 7,
    StartTicketDiscussion = 8,
    StopTicketDiscussion = 9,
    WhoAmI = 10,
};

struct TCommand
{
    ECommand Type = ECommand::Unknown;
    TVector<TString> Args;

    static TCommand Parse(const TString& s)
    {
        TCommand command;

        TStringBuf it(s);
        TStringBuf tok;

        it.NextTok(' ', tok);
        if (!tok.StartsWith('/')) {
            return command;
        }

        tok.Skip(1);
        if (tok == "eval_ticket") {
            command.Type = ECommand::EvalTicket;
        } else if (tok == "list_tickets") {
            command.Type = ECommand::ListMyTickets;
        } else if (tok == "start_ticket_evaluation") {
            command.Type = ECommand::StartTicketEvaluation;
        } else if (tok == "stop_ticket_evaluation") {
            command.Type = ECommand::StopTicketEvaluation;
        } else if (tok == "make_plan") {
            command.Type = ECommand::MakePlan;
        } else if (tok == "start") {
            command.Type = ECommand::Start;
        } else if (tok == "help") {
            command.Type = ECommand::Help;
        } else if (tok == "start_ticket_discussion") {
            command.Type = ECommand::StartTicketDiscussion;
        } else if (tok == "stop_ticket_discussion") {
            command.Type = ECommand::StopTicketDiscussion;
        } else if (tok == "whoami") {
            command.Type = ECommand::WhoAmI;
        } else {
            throw yexception() << "no such command: " << tok;
        }

        TVector<TString> args;
        while (it.NextTok(' ', tok)) {
            command.Args.push_back(TString(tok));
        }

        return command;
    }
};

////////////////////////////////////////////////////////////////////////////////

// does not take holidays into account
class TSimpleCalendar
{
public:
    TSimpleCalendar()
    {
        // XXX hardcode
        const TInstant b = TInstant::ParseIso8601("2021-01-01");   // friday
        const TInstant e = TInstant::ParseIso8601("2024-01-01");

        auto i = b;
        i += TDuration::Days(1);
        while (i < e) {
            SkipDays.push_back(i);
            SkipDays.push_back(i + TDuration::Days(1));

            i += TDuration::Days(7);
        }
    }

public:
    TInstant Deadline(const TInstant baseDate, const TDuration duration)
    {
        auto it = LowerBound(SkipDays.begin(), SkipDays.end(), baseDate);
        TDuration workDays;
        TDuration skipDays;
        while (workDays < duration) {
            TInstant d = baseDate + workDays + skipDays;
            if (it == SkipDays.end()) {
                workDays += duration - workDays;
            } else if (d == *it) {
                skipDays += TDuration::Days(1);
                ++it;
            } else {
                workDays += *it - d;
            }
        }

        return baseDate + duration + skipDays;
    }

private:
    TVector<TInstant> SkipDays;
};

////////////////////////////////////////////////////////////////////////////////

TString TicketUrl(const TString& ticketKey)
{
    return "https://st.yandex-team.ru/" + ticketKey;
}

////////////////////////////////////////////////////////////////////////////////

enum class ETicketType
{
    Dev = 0,
    Sre = 1,
    Tests = 2,
};

enum class ETicketPriority
{
    Minor = 0,
    Normal = 1,
    Critical = 2,
    Blocker = 3,
};

enum class ETicketStatus
{
    Open = 0,
    InProgress = 1,
    Other = 2,
};

struct TTicket
{
    TString Key;
    TString Title;
    TString Description;
    TString Assignee;
    ETicketStatus Status = ETicketStatus::Other;
    ETicketType TicketType = ETicketType::Dev;
    ETicketPriority Priority = ETicketPriority::Minor;
    bool Reassignable = false;
    TInstant OriginalDeadline;
};

TString Ticket2String(const TTicket& ticket)
{
    TStringBuilder sb;
    sb << TicketUrl(ticket.Key) << "\n";
    sb << "Title: " << ticket.Title << "\n";
    sb << "Description: " << ticket.Description << "\n";
    sb << "Current Assignee: " << ticket.Assignee << "\n";
    TString ticketStatusStr;
    switch (ticket.Status) {
        case ETicketStatus::Open: ticketStatusStr = "open"; break;
        case ETicketStatus::InProgress: ticketStatusStr = "inProgress"; break;
        case ETicketStatus::Other: ticketStatusStr = "other"; break;
        default: Y_VERIFY(0);
    }
    sb << "Status: " << ticketStatusStr << "\n";
    TString ticketTypeStr;
    switch (ticket.TicketType) {
        case ETicketType::Dev: ticketTypeStr = "dev"; break;
        case ETicketType::Sre: ticketTypeStr = "SRE"; break;
        case ETicketType::Tests: ticketTypeStr = "tests"; break;
        default: Y_VERIFY(0);
    }
    sb << "Type: " << ticketTypeStr << "\n";
    TString priorityStr;
    switch (ticket.Priority) {
        case ETicketPriority::Minor: priorityStr = "minor"; break;
        case ETicketPriority::Normal: priorityStr = "normal"; break;
        case ETicketPriority::Critical: priorityStr = "critical"; break;
        case ETicketPriority::Blocker: priorityStr = "blocker"; break;
        default: Y_VERIFY(0);
    }
    sb << "Priority: " << priorityStr << "\n";
    sb << "Reassignable: " << ticket.Reassignable << "\n";
    sb << "OriginalDeadline: " << ticket.OriginalDeadline.ToIsoStringLocal();
    return sb;
}

////////////////////////////////////////////////////////////////////////////////

struct TSTClient
{
    const TString StToken;
    mutable TKeepAliveHttpClient Client;
    TMutex ClientLock;

    TSTClient(TString stToken)
        : StToken(std::move(stToken))
        , Client("https://st-api.yandex-team.ru", 443)
    {
    }

    auto BuildHeaders() const
    {
        TKeepAliveHttpClient::THeaders headers;
        headers["Authorization"] = "OAuth " + StToken;
        return headers;
    }

    NJson::TJsonValue Fetch(const TString& url) const
    {
        auto g = Guard(ClientLock);

        auto headers = BuildHeaders();
        TStringStream ss;
        auto code = Client.DoGet(url, &ss, headers);

        if (code / 100 != 2) {
            throw yexception() << "bad http code: " << code
                << ", output: " << ss.Str();
        }

        NJson::TJsonValue v;
        NJson::ReadJsonTree(ss.Str(), &v, true);

        return v;
    }

    void UpdateTicket(
        const TString& ticketKey,
        const TString& user,
        TInstant deadline)
    {
        auto g = Guard(ClientLock);

        auto headers = BuildHeaders();
        TStringStream ss;
        NJson::TJsonValue body;
        body["assignee"]["id"] = user;
        body["deadline"] = deadline.FormatLocalTime("%Y-%m-%d");
        TStringStream bodyStr;
        NJson::WriteJson(&bodyStr, &body);
        auto code = Client.DoRequest(
            "PATCH",
            "/v2/issues/" + ticketKey,
            bodyStr.Str(),
            &ss,
            headers
        );

        if (code / 100 != 2) {
            throw yexception() << "bad http code: " << code
                << ", output: " << ss.Str();
        }

        NJson::TJsonValue v;
        NJson::ReadJsonTree(ss.Str(), &v, true);
    }

    void FetchTickets(
        ui32 page,
        const TString& queue,
        TVector<TTicket>& tickets) const
    {
        auto v = Fetch(Sprintf(
            "/v2/issues?perPage=100&page=%u&filter=queue:%s",
            page,
            queue.c_str()
        ));

        for (const auto& x: v.GetArraySafe()) {
            if (x["type"]["key"].GetString() == "epic") {
                continue;
            }

            const auto& status = x["status"]["key"].GetString();

            if (status == "commited" || status == "closed") {
                continue;
            }

            THashSet<TString> tags;
            for (const auto& tag: x["tags"].GetArray()) {
                tags.insert(tag.GetString());
            }

            bool isNew = false;
            const auto& createdAt = x["createdAt"].GetString();
            if (createdAt) {
                auto createdAtTs = TInstant::ParseIso8601(createdAt);
                const auto threshold = TInstant::ParseIso8601("2021-03-01");
                isNew = createdAtTs > threshold;
            }

            if (tags.contains("release")) {
                continue;
            }

            if (!x.Has("deadline") && !tags.contains("important") && !isNew) {
                continue;
            }

            const auto& priority = x["priority"]["key"].GetString();

            TTicket ticket;
            ticket.Key = x["key"].GetString();
            ticket.Title = x["summary"].GetString();
            ticket.Description = x["description"].GetString();
            if (ticket.Description.Size() > 400) {
                ticket.Description.resize(400);
                ticket.Description += "...";
            }
            if (status == "open") {
                ticket.Status = ETicketStatus::Open;
            } else if (status == "inProgress") {
                ticket.Status = ETicketStatus::InProgress;
            } else {
                ticket.Status = ETicketStatus::Other;
            }
            ticket.Assignee = x["assignee"]["id"].GetString();
            if (tags.contains("dev")) {
                ticket.TicketType = ETicketType::Dev;
            } else if (tags.contains("SRE")) {
                ticket.TicketType = ETicketType::Sre;
            } else if (tags.contains("tests")) {
                ticket.TicketType = ETicketType::Tests;
            }
            if (priority == "blocker") {
                ticket.Priority = ETicketPriority::Blocker;
            } else if (priority == "critical") {
                ticket.Priority = ETicketPriority::Critical;
            } else if (priority == "normal") {
                ticket.Priority = ETicketPriority::Normal;
            } else {
                ticket.Priority = ETicketPriority::Minor;
            }
            ticket.Reassignable =
                !tags.contains("assigned") && !tags.contains("duty");
            const auto& originalDeadline = x["deadline"].GetString();
            if (originalDeadline) {
                ticket.OriginalDeadline =
                    TInstant::ParseIso8601(originalDeadline);
            }
            // Cdbg << "parsed ticket: " << Ticket2String(ticket) << Endl;
            tickets.push_back(std::move(ticket));
        }

        if (v.GetArraySafe().size() == 100) {
            FetchTickets(page + 1, queue, tickets);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

// TODO: move User2XXX to TUserStates
// this class should only contain a sorted list of tickets

struct TTickets: IThreadFactory::IThreadAble
{
    struct TUnscoredTicket
    {
        TTicket* Ticket = nullptr;
        TString Message;
    };

    TAtomic Running;

    TSTClient& StClient;

    TVector<TTicket> Tickets;
    THashMap<TString, TInstant> OriginalDeadlines;
    THashMap<TString, TVector<TUnscoredTicket>> User2Unscored;
    THashMap<TString, THashSet<TString>> User2Scored;

    TMutex Lock;
    TManualEvent Ev;

    TTickets(TSTClient& stClient)
        : StClient(stClient)
    {
        SyncTickets();
        AtomicSet(Running, true);
        SystemThreadFactory()->Run(this);
    }

    ~TTickets()
    {
        AtomicSet(Running, false);
        Ev.WaitI();
    }

    void ResetScores()
    {
        with_lock (Lock) {
            User2Unscored.clear();
            User2Scored.clear();
        }
    }

    void MarkScored(
        const TStringBuf login,
        const TStringBuf ticketKey,
        TInstant scoreTs)
    {
        with_lock (Lock) {
            auto& unscored = User2Unscored[login];
            auto& scored = User2Scored[login];

            InitUserIfNeeded(login, unscored, scored);

            auto it = FindIf(
                unscored.begin(),
                unscored.end(),
                [=] (const TUnscoredTicket& t) {
                    return t.Ticket->Key == ticketKey;
                }
            );

            if (it == unscored.end()) {
                Cdbg << "ticket " << ticketKey
                    << " for user " << login
                    << " not found" << Endl;
                return;
            }

            if (auto dl = OriginalDeadlines.FindPtr(ticketKey)) {
                if (*dl < Now() + CLOSE_DEADLINE_INTERVAL
                        && scoreTs < Now() - REEVALUATION_INTERVAL)
                {
                    it->Message = "old score and an [almost]expired deadline"
                        " - reevaluation needed";
                    Cdbg << "ticket " << ticketKey << " for user " << login
                        << " has " << it->Message << Endl;
                    return;
                }
            }

            Y_VERIFY(scored.insert(TString(ticketKey)).second);

            auto d = std::distance(unscored.begin(), it);
            long l = unscored.size() - 1;
            if (d != l) {
                DoSwap(unscored[d], unscored[l]);
            }
            unscored.pop_back();
            Cdbg << "removed ticket " << ticketKey
                << " for user " << login
                << " from unscored" << Endl;
        }
    }

    ui32 NextUnscoredTicket(
        const TStringBuf login,
        TTicket* ticket,
        TString* message)
    {
        with_lock (Lock) {
            auto& unscored = User2Unscored[login];
            auto& scored = User2Scored[login];

            InitUserIfNeeded(login, unscored, scored);

            if (unscored.empty()) {
                return 0;
            }

            *ticket = *unscored.front().Ticket;
            *message = unscored.front().Message;
            return unscored.size();
        }
    }

    TVector<TTicket> GetTickets() const
    {
        with_lock (Lock) {
            return Tickets;
        }
    }

private:
    void InitUserIfNeeded(auto login, auto& unscored, auto& scored)
    {
        if (unscored.empty() && scored.empty()) {
            for (auto& t: Tickets) {
                if (IsAssignable(t, login)) {
                    unscored.push_back({&t, TString()});
                }
            }
        }
    }

    void DoExecute() override
    {
        while (AtomicGet(Running)) {
            SyncTickets();
            Sleep(TDuration::Minutes(1));
        }

        Ev.Signal();
    }

    void SyncTickets()
    {
        try {
            TVector<TTicket> tickets;
            StClient.FetchTickets(1, "NBSOPS", tickets);
            StClient.FetchTickets(1, "NBS", tickets);

            with_lock (Lock) {
                THashMap<TString, THashMap<TString, TString>> messages;

                for (auto& x: User2Unscored) {
                    for (const auto& y: x.second) {
                        if (y.Message) {
                            messages[x.first][y.Ticket->Key] =
                                std::move(y.Message);
                        }
                    }
                    x.second.clear();
                }

                Tickets = std::move(tickets);
                for (auto& ticket: Tickets) {
                    for (auto& x: User2Unscored) {
                        auto& y = User2Scored[x.first];
                        if (!y.contains(ticket.Key)
                                && IsAssignable(ticket, x.first))
                        {
                            TString message;
                            if (auto p = messages.FindPtr(x.first)) {
                                if (auto pp = p->FindPtr(ticket.Key)) {
                                    message = *pp;
                                }
                            }
                            x.second.push_back({&ticket, std::move(message)});
                        }
                    }

                    OriginalDeadlines[ticket.Key] = ticket.OriginalDeadline;
                }
            }
        } catch (const yexception& e) {
            Cerr << Now() << "\terror: " << e.what() << Endl;
        }
    }

private:
    static bool IsAssignable(const TTicket& ticket, const TStringBuf login)
    {
        return  ticket.Status == ETicketStatus::Open && ticket.Reassignable
            || ticket.Assignee == login;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TUserStates
{
    struct TUserState
    {
        TTicket TicketToEvaluate;
    };

    TMutex Lock;
    THashMap<TString, TUserState> User2State;

    TUserStates()
    {
    }

    void SetTicketToEvaluate(const TStringBuf login, TTicket ticket)
    {
        with_lock (Lock) {
            User2State[login].TicketToEvaluate = std::move(ticket);
        }
    }

    TTicket GetTicketToEvaluate(const TStringBuf login) const
    {
        with_lock (Lock) {
            if (auto p = User2State.FindPtr(login)) {
                return p->TicketToEvaluate;
            }
        }

        return {};
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTicketScoreStorage
{
    struct TScore
    {
        TInstant Ts;
        ui32 Score = 0;
    };

    struct TUserScores
    {
        TMap<TString, TScore> TicketKey2Score;
    };

    TFile File;
    TMutex Lock;
    THashMap<TString, TUserScores> User2Scores;

    TTicketScoreStorage(const TString& logPath)
        : File(logPath, OpenAlways | WrOnly | ForAppend | Seq | NoReuse)
    {
        TIFStream is(logPath);
        TString line;
        while (is.ReadLine(line)) {
            TStringBuf it(line);
            auto login = it.NextTok(' ');
            TString ticketKey(it.NextTok(' '));
            auto scoreStr = it.NextTok(' ');
            auto tsStr = it.NextTok(' ');
            if (!tsStr) {
                tsStr = "2021-01-01";
            }
            auto ts = TInstant::ParseIso8601(tsStr);

            auto& ticketKey2Score = User2Scores[login].TicketKey2Score;
            ticketKey2Score[ticketKey] = {ts, FromString<ui32>(scoreStr)};
        }
    }

    void RegisterScore(
        const TString& login,
        const TString& ticketKey,
        const TScore& score)
    {
        if (score.Score <= 0
                || score.Score > 64
                || (score.Score & (score.Score - 1)) != 0)
        {
            throw yexception() << "invalid score: " << score.Score;
        }

        auto validateString = [] (const TStringBuf s) {
            for (const auto& c: s) {
                if (!IsAlnum(c) && c != '-') {
                    return false;
                }
            }

            return true;
        };

        if (!validateString(login)) {
            throw yexception() << "invalid login: " << login;
        }

        if (!validateString(ticketKey)) {
            throw yexception() << "invalid ticket: " << ticketKey;
        }

        with_lock (Lock) {
            User2Scores[login].TicketKey2Score[ticketKey] = score;
            TStringBuilder sb;
            sb << login
                << ' ' << ticketKey
                << ' ' << score.Score
                << ' ' << score.Ts.ToIsoStringLocal()
                << '\n';
            File.Write(sb.Data(), sb.Size());
        }
    }

    TMap<TString, TScore> GetUserScores(const TStringBuf login) const
    {
        with_lock (Lock) {
            if (auto p = User2Scores.FindPtr(login)) {
                return p->TicketKey2Score;
            }
        }

        return {};
    }

    THashMap<TString, TUserScores> GetUserScores() const
    {
        with_lock (Lock) {
            return User2Scores;
        }
    }

    using TVisitor = std::function<void(
        const TString& login,
        const TString& ticketKey,
        const TScore& score
    )>;

    void Visit(const TVisitor& visitor) const
    {
        with_lock (Lock) {
            for (const auto& x: User2Scores) {
                for (const auto& y: x.second.TicketKey2Score) {
                    visitor(x.first, y.first, y.second);
                }
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTicketDiscussionState
{
    TDeque<TTicket> ToDiscussQueue;
    TVector<TTicket> DiscussionNeeded;
};

////////////////////////////////////////////////////////////////////////////////

struct TScheduledTicket
{
    TTicket Ticket;
    TDuration Score;
    TInstant Deadline;
};

struct TSchedule
{
    TVector<TScheduledTicket> Tickets;
    TDuration Length;
};

struct TPlan
{
    TInstant Date;
    TMap<TString, TSchedule> User2Schedule;
};

TPlan MakePlan(
    const TVector<TTicket>& tickets,
    const THashMap<TString, TTicketScoreStorage::TUserScores>& user2Scores,
    const THashSet<TString>& assigneeFilter)
{
    TPlan plan;
    plan.Date = TInstant::Days(Now().Days());

    TSimpleCalendar calendar;

    const auto defaultScore = TDuration::Days(365);
    auto getScore = [&] (const TString& user, const TString& ticketKey) {
        if (auto p = user2Scores.FindPtr(user)) {
            if (auto pp = p->TicketKey2Score.FindPtr(ticketKey)) {
                return pp->Score * SCORE_UNIT;
            }
        }

        Cerr << "warning: no score for user " << user << " and ticket " << ticketKey << Endl;

        return defaultScore;
    };

    TVector<TTicket> reassignable;
    TDuration longest;
    TVector<TString> errors;
    for (const auto& ticket: tickets) {
        if (ticket.Status == ETicketStatus::Open && ticket.Reassignable) {
            reassignable.push_back(ticket);
        } else {
            auto& s = plan.User2Schedule[ticket.Assignee];
            const auto score = getScore(ticket.Assignee, ticket.Key);
            if (score == defaultScore) {
                errors.push_back(TStringBuilder() << "ticket " << ticket.Key
                    << " not scored by " << ticket.Assignee);
                continue;
            }
            s.Length += score;
            s.Tickets.push_back({ticket, score, {}});

            longest = Max(longest, s.Length);
        }
    }

    if (errors) {
        TStringBuilder exc;
        for (const auto& error: errors) {
            if (exc.Size() + error.Size() > 1024) {
                break;
            }

            if (exc.Size()) {
                exc << "; ";
            }
            exc << error;
        }

        throw yexception() << exc;
    }

    while (reassignable.size()) {
        TString bestUser;
        ui32 bestTicket = 0;
        TDuration bestLength = TDuration::Max();
        TDuration bestScore = TDuration::Max();
        for (ui32 i = 0; i < reassignable.size(); ++i) {
            auto& ticket = reassignable[i];

            for (auto& x: user2Scores) {
                if (!assigneeFilter.contains(x.first)) {
                    continue;
                }

                auto& s = plan.User2Schedule[x.first];
                const auto score = getScore(x.first, ticket.Key);
                auto length = s.Length + score;
                if (length < bestLength) {
                    bestLength = length;
                    bestScore = score;
                    bestUser = x.first;
                    bestTicket = i;
                }
            }
        }

        auto& bestS = plan.User2Schedule[bestUser];
        bestS.Length = bestLength;
        bestS.Tickets.push_back({
            reassignable[bestTicket],
            bestScore,
            {}
        });

        longest = Max(longest, bestS.Length);
        Cdbg << "Assigned ticket " << reassignable[bestTicket].Key
            << " to user " << bestUser
            << " with score " << bestScore
            << ", schedule length is now " << bestLength << Endl;
        Cdbg << "Longest schedule is now " << longest << Endl;

        DoSwap(reassignable[bestTicket], reassignable.back());
        reassignable.pop_back();
    }

    for (auto& x: plan.User2Schedule) {
        auto& userTickets = x.second.Tickets;
        StableSort(
            userTickets.begin(),
            userTickets.end(),
            [] (const TScheduledTicket& l, const TScheduledTicket& r) {
                bool lInProgress = l.Ticket.Status == ETicketStatus::InProgress;
                bool rInProgress = r.Ticket.Status == ETicketStatus::InProgress;
                return lInProgress != rInProgress
                    ? lInProgress
                    : l.Ticket.Priority > r.Ticket.Priority;
            }
        );

        TInstant deadline = plan.Date;
        for (auto& t: userTickets) {
            deadline = calendar.Deadline(deadline, t.Score);
            t.Deadline = deadline;
        }
    }

    return plan;
}

////////////////////////////////////////////////////////////////////////////////

void DumpPlan(const TPlan& plan, IOutputStream& os)
{
    bool first = true;
    for (const auto& x: plan.User2Schedule) {
        if (!first) {
            os << "\n";
        }
        os << "User: " << x.first << "\n";
        for (const auto& t: x.second.Tickets) {
            os << "================================" << "\n";
            os << "Ticket: " << Ticket2String(t.Ticket) << "\n";
            os << "Deadline: " << t.Deadline << "\n";
        }

        first = false;
    }
}

////////////////////////////////////////////////////////////////////////////////

void ApplyPlan(const TPlan& plan, TSTClient& stClient)
{
    for (const auto& x: plan.User2Schedule) {
        for (const auto& t: x.second.Tickets) {
            stClient.UpdateTicket(t.Ticket.Key, x.first, t.Deadline);
        }
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

struct TAutoTeamlead::TImpl
{
    TSTClient StClient;
    THolder<TTicketScoreStorage> TicketScores;
    THolder<TUserStates> UserStates;
    THolder<TTickets> Tickets;
    THolder<TTicketDiscussionState> DiscussionState;
    TMutex RevisitLock;
    TInstant RevisitTs;

    THashSet<TString> AssigneeFilter;
    THashSet<TString> Admins;

    TMutex StateLock;

    TImpl(const TString& stToken, const NJson::TJsonValue& config)
        : StClient(stToken)
    {
        TString scoreLogPath = config["score-log"].GetString();
        if (!scoreLogPath) {
            scoreLogPath = "scores.log";
        }

        TicketScores.Reset(new TTicketScoreStorage(scoreLogPath));
        UserStates.Reset(new TUserStates());
        Tickets.Reset(new TTickets(StClient));

        TString assigneesPath = config["assignees"].GetString();
        if (!assigneesPath) {
            assigneesPath = "assignees.txt";
        }

        const auto& admins = config["admins"];
        if (admins.IsArray()) {
            for (const auto& admin: admins.GetArray()) {
                Admins.insert(admin.GetString());
            }
        } else {
            Admins.insert("qkrorlqr");
        }

        {
            TIFStream is(assigneesPath);
            TString assignee;
            while (is.ReadLine(assignee)) {
                AssigneeFilter.insert(assignee);
            }
        }

        RevisitScoresIfNeeded();
    }

    void RevisitScoresIfNeeded()
    {
        with_lock (RevisitLock) {
            auto now = Now();

            if (RevisitTs + REEVALUATION_INTERVAL < now) {
                Tickets->ResetScores();

                TicketScores->Visit(
                    [&] (
                        const TString& login,
                        const TString& ticketKey,
                        const TTicketScoreStorage::TScore& score)
                    {
                        Tickets->MarkScored(login, ticketKey, score.Ts);
                    }
                );

                RevisitTs = now;
            }
        }
    }

    void VerifyAdmin(const TString& login) const
    {
        Y_ENSURE(
            Admins.contains(login),
            Sprintf(
                "user %s is not allowed to perform this operation",
                login.c_str()
            )
        );
    }

    bool SelectAnswer(
        const TString& login,
        const TString& message,
        const TString& chatId,
        TString* answer,
        TString* sticker)
    {
        auto g = Guard(StateLock);

        RevisitScoresIfNeeded();

        try {
            auto command = TCommand::Parse(message);

            if (command.Type == ECommand::Unknown && DiscussionState) {
                auto& t = DiscussionState->ToDiscussQueue.front();

                if (t.Assignee != login && !Admins.contains(login)) {
                    TStringBuilder sb;
                    sb << "need response from one of: " << t.Assignee;
                    for (const auto& admin: Admins) {
                        sb << ", " << admin;
                    }

                    *answer = sb;
                    return true;
                }

                bool discuss;
                if (TryFromString<bool>(message, discuss)) {
                    if (discuss) {
                        DiscussionState->DiscussionNeeded.push_back(std::move(t));
                    }

                    DiscussionState->ToDiscussQueue.pop_front();
                    if (DiscussionState->ToDiscussQueue.size()) {
                        *answer = TStringBuilder()
                            << Ticket2String(DiscussionState->ToDiscussQueue.front())
                            << "\ntickets left: " << DiscussionState->ToDiscussQueue.size();
                        return true;
                    } else {
                        command.Type = ECommand::StopTicketDiscussion;
                    }
                }
            }

            bool isInTicketEvaluationMode = false;
            if (command.Type == ECommand::Unknown) {
                auto ticket = UserStates->GetTicketToEvaluate(login);
                if (!ticket.Key) {
                    return false;
                }

                isInTicketEvaluationMode = true;

                command.Type = ECommand::EvalTicket;
                command.Args = {ticket.Key, message};
            }

            switch (command.Type) {
                case ECommand::EvalTicket: {
                    Y_ENSURE(
                        command.Args.size() >= 2,
                        "bad argument count: " << command.Args.size()
                    );

                    TString user = login;
                    if (command.Args.size() == 3) {
                        VerifyAdmin(login);
                        user = command.Args[2];
                        Y_VERIFY(!isInTicketEvaluationMode);
                    }

                    const TTicketScoreStorage::TScore score{
                        Now(),
                        FromString<ui32>(command.Args[1])
                    };
                    TicketScores->RegisterScore(user, command.Args[0], score);

                    Tickets->MarkScored(user, command.Args[0], score.Ts);

                    *answer = TStringBuilder() << "registered score "
                        << command.Args[1] << " for user " << user
                        << " and ticket " << TicketUrl(command.Args[0]);

                    if (isInTicketEvaluationMode) {
                        TTicket ticket;
                        TString details;
                        auto unscoredCount =
                            Tickets->NextUnscoredTicket(user, &ticket, &details);
                        if (unscoredCount) {
                            UserStates->SetTicketToEvaluate(user, ticket);
                            TStringBuilder sb;
                            sb << Ticket2String(ticket);
                            if (details) {
                                sb << " (" << details << ")";
                            }
                            sb << "\n remaining tickets: " << unscoredCount;
                            *answer = sb;
                        } else {
                            *answer = "you have already scored all tickets";
                            *sticker = REST_STICKER;
                            UserStates->SetTicketToEvaluate(user, {});
                        }
                    }

                    break;
                }

                case ECommand::ListMyTickets: {
                    auto scores = TicketScores->GetUserScores(login);
                    for (const auto& x: scores) {
                        if (answer->Size()) {
                            *answer += "\n";
                        }

                        *answer += TStringBuilder() << TicketUrl(x.first)
                            << " score=" << x.second.Score
                            << " (" << x.second.Ts << ")";
                    }

                    break;
                }

                case ECommand::StartTicketEvaluation: {
                    // TODO: a mode for ticket reevaluation - for tickets with missed deadlines

                    TTicket ticket;
                    TString message;
                    auto unscoredCount =
                        Tickets->NextUnscoredTicket(login, &ticket, &message);
                    if (unscoredCount) {
                        UserStates->SetTicketToEvaluate(login, ticket);
                        *sticker = TOUGH_CHOICE_STICKER;
                        TStringBuilder sb;
                        sb << "Assign a score for each ticket"
                            << ", allowed scores: 1, 2, 4, 8, 16, 32, 64"
                            << ", 1 score unit = " << SCORE_UNIT << "\n\n"
                            << Ticket2String(ticket);
                        if (message) {
                            sb << " (" << message << ")";
                        }
                        sb << "\n remaining tickets: " << unscoredCount;
                        *answer = sb;
                    } else {
                        *answer = "you have already scored all tickets";
                        *sticker = REST_STICKER;
                        UserStates->SetTicketToEvaluate(login, {});
                    }

                    break;
                }

                case ECommand::StopTicketEvaluation: {
                    *sticker = REST_STICKER;
                    UserStates->SetTicketToEvaluate(login, {});

                    break;
                }

                case ECommand::StartTicketDiscussion: {
                    VerifyAdmin(login);

                    if (DiscussionState) {
                        *answer = "discussion already started";
                    } else {
                        DiscussionState.Reset(new TTicketDiscussionState);

                        auto tickets = Tickets->GetTickets();
                        auto now = Now();
                        for (auto& t: tickets) {
                            if (t.Priority == ETicketPriority::Blocker) {
                                DiscussionState->DiscussionNeeded.push_back(std::move(t));
                            } else if (t.OriginalDeadline < now + DISCUSSION_INTERVAL) {
                                DiscussionState->ToDiscussQueue.push_back(std::move(t));
                            }
                        }

                        SortBy(DiscussionState->ToDiscussQueue, [] (const auto& t) {
                            return t.Assignee;
                        });

                        if (DiscussionState->ToDiscussQueue.empty()) {
                            *answer = "nothing to discuss";
                            DiscussionState.Reset();
                        } else {
                            TStringBuilder sb;
                            sb << Ticket2String(DiscussionState->ToDiscussQueue.front());
                            sb << "\n" << "blockers: " << DiscussionState->DiscussionNeeded.size()
                                << ", queue: " << DiscussionState->ToDiscussQueue.size();
                            sb << "\n" << "1 - discussion needed, 0 - skip";
                            *answer = sb;
                        }
                    }

                    break;
                }

                case ECommand::StopTicketDiscussion: {
                    if (DiscussionState) {
                        TStringBuilder sb;
                        sb << "Discussion needed for tickets:";
                        for (const auto& t: DiscussionState->DiscussionNeeded) {
                            sb << "\n";
                            sb << TicketUrl(t.Key);
                        }
                        *answer = sb;

                        DiscussionState.Reset();
                    } else {
                        *answer = "discussion not started yet";
                    }

                    break;
                }

                case ECommand::MakePlan: {
                    VerifyAdmin(login);

                    auto tickets = Tickets->GetTickets();
                    auto userScores = TicketScores->GetUserScores();
                    auto plan = MakePlan(tickets, userScores, AssigneeFilter);

                    TFsPath path(TStringBuilder() << "plan_"
                        << plan.Date.FormatLocalTime("%Y_%b_%dT%H_%M")
                        << ".txt");

                    TOFStream fs(path.GetPath());
                    DumpPlan(plan, fs);
                    if (command.Args.size() && command.Args[0] == "apply") {
                        ApplyPlan(plan, StClient);
                    }

                    // TODO: upload to paste.yandex-team.ru
                    *answer = TStringBuilder() << "Dumped to " << path.GetPath();

                    break;
                }

                case ECommand::Start: {
                    *sticker = START_STICKER;
                    *answer = "type /help to see the list of commands";

                    break;
                }

                case ECommand::Help: {
                    *answer = HELP_MESSAGE;

                    break;
                }

                case ECommand::WhoAmI: {
                    *answer = chatId;
                    break;
                }

                default: {
                    Y_VERIFY(0);
                }
            }
        } catch (const yexception& e) {
            Cerr << Now() << "\terror: " << e.what() << Endl;
            *answer = TStringBuilder() << "got error: " << e.what()
                << "\nhelp:\n" << HELP_MESSAGE;
        }

        return true;
    }
};

TAutoTeamlead::TAutoTeamlead(
        const TString& stToken,
        const NJson::TJsonValue& config)
    : Impl(new TImpl(stToken, config))
{
}

TAutoTeamlead::~TAutoTeamlead()
{
}

bool TAutoTeamlead::SelectAnswer(
    const TString& login,
    const TString& message,
    const TString& chatId,
    TString* answer,
    TString* sticker)
{
    return Impl->SelectAnswer(login, message, chatId, answer, sticker);
}

}   // namespace NCoolBot
