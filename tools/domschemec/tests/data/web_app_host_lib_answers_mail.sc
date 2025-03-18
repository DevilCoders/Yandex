namespace NAppHost::NMail;

struct TEmailAddress {
    local(required): string;
    domain(required): string;
    display_name(required): string;
};

struct TUser {
    uid(required): ui64;
    db: string;
    suid: string;
    email(required): TEmailAddress;
    country: string;
    is_phone_confirmed: bool;
    born_date: i64;
    karma: i32;
    karma_status: i32;
};

struct TLabel {
    name(required): string;
    type(required): string;
};

struct TFolderPath {
    path: string;
    delim: string;
};

struct TFolderCoords {
    fid: string;
    path: TFolderPath;
};

struct TParams {
    old_mid: string;
    bcc: [TEmailAddress];
    folder: TFolderCoords;
    lids: [string];
    label_symbols: [string];
    labels: [TLabel];
    spam: bool;
    use_filters: bool (default = true);
    external_imap_id: ui32;
    disable_push: bool (default = false);
    store_as_deleted: bool (default = false);
    ignore_duplicates: bool (default = false);
    remove_duplicates: bool (default = false);
    no_such_folder_action: string (default = "fallback_to_inbox"); // one of "fail", "fallback_to_inbox", "create"
};

struct TRecipient {
    user(required): TUser;
    params: TParams;
};

struct TEnvelope {
    remote_ip(required): string;
    remote_host(required): string;
    helo(required): string;
    mail_from(required): string;
    mail_from_user: TUser;
    received_date(required): i64;
};

struct TPart {
    content_type(required): string;
    content_subtype(required): string;
    boundary: string;
    name: string;
    charset: string;
    encoding: string;
    content_disposition: string;
    file_name: string;
    content_id: string;
    offset: ui64;
    length: ui64;
    data: string;
};

struct TAttachment {
    name(required): string;
    type(required): string;
    size(required): ui64;
};

struct TMessage {
    subject(required): string;
    from(required): [TEmailAddress];
    to(required): [TEmailAddress];
    cc: [TEmailAddress];
    spam: bool;
    parts: {string -> TPart};
    attachments: {string -> TAttachment}; 
    size: i64;
    service: string;
    auth_results: string;
    message_id: string;
    sender: string;
    reply_to: string;
    in_reply_to: string;
    references: [string];
    date: i64;
};

struct TRequest {
    message(required): TMessage;
    envelope(required): TEnvelope;
    recipients: {string -> TRecipient};
    session_id(required): string;
    stid(required): string;
    domain_label: string;
    types(required): [i32];
};

struct TShingerPrint {
    uids(required): [ui64];
    session_id(required): string;
    part_id(required): string;
    shingers(required): [ui64];
    token_indices: [ui64];
};

struct TTemplateMatchInfo {
    deltas: [[string]];
    similarity(required): double;
    stable_sign(required): ui64;
};

struct TTemplateIdentifier {
    uids(required): [ui64];
    stid: string;
    session_id: string;
    templates(required): [TTemplateMatchInfo];
};

struct TFeature {
    title(required): string;
    value(required): double;
};

struct TFeatures {
    features(required): [TFeature];
};

struct TModel {
    user_experiments(required): [ui32];
    model_type(required): string;
};

struct TApplyModel {
    models(required): [TModel];
};

struct TExperiment {
    test_id(required): i32;
    bucket(required): i32;
};

struct TUaas {
    user_experiments(required): [TExperiment];
};

struct TUaasProxyMeta {
    headers(required): [[string]];
};

struct TModelResult {
    result(required): string;
    model_type(required): string;
    model_id: string;
    model_date: string;
    model_predict: double;
    model_class: i32;
    error_message: string;
};

struct TMxnetApply {
    mxnet_result(required): [TModelResult];
};

struct TAction {
    verified: bool;
    parameter: string;
    type(required): string;
};

struct TCondition {
    field_type(required): string;
    div: string;
    link: string;
    oper: i32;
    pattern: string;
};

struct TRule {
    id(required): string;
    type(required): string;
    created(required): ui64;
    name(required): string;
    priority(required): i64;
    query(required): string;
    enabled(required): bool;
    stop(required): bool;
    actions(required): [TAction];
    conditions(required): [TCondition];
};

struct TFuritaUser {
    uid(required): ui64;
    result(required): string;
    message: string; // Message with error description
    rules: [TRule]; 
};

struct TFurita {
    session(required): string;
    users(required): [TFuritaUser];
};

struct TFuritaSuccessUser {
    uid(required): ui64;
    rules: [TRule];
};

struct TFuritaSuccessUsers {
    session(required): string;
    users(required): [TFuritaSuccessUser];
};

struct TTupitaUserWithMatchedQueries {
    matched_queries(required): [string];
    uid(required): ui64;
};

struct TTupita {
    result(required): [TTupitaUserWithMatchedQueries];
};

struct TTupitaPostDataQuery {
    id(required): string;
    query(required): string;
    stop(required): bool;
};

struct TTupitaPostDataUserWithQueries {
    uid(required): ui64;
    queries(required): [TTupitaPostDataQuery]; 
    spam: bool;
};

struct TTupitaPostDataMessage {
    subject(required): string;
    from(required): [TEmailAddress];
    to(required): [TEmailAddress];
    cc: [TEmailAddress];
    stid(required): string;
    spam(required): bool;
    types(required): [i32];
    attachmentsCount: ui64 (default = 0);
};

struct TTupitaPostData {
    message(required): TTupitaPostDataMessage;
    users(required): [TTupitaPostDataUserWithQueries];
};

struct TTupitaBuildRequest {
    post_data(required): string;
    cgi_params(required): string;
};

struct TFuritaBuildRequest {
    cgi_params(required): string;
};

struct TUaasBuildRequest {
    cgi_params(required): string;
};

struct TForward {
    address(required): string;
};

struct TAutoreply {
    address(required): string;
    body(required): string;
};

struct TNotify {
    address(required): string;
};

struct TAppliedRules {
    notifies: [TNotify];
    replies: [TAutoreply];
    forwards: [TForward];
    dest_folder: TFolderCoords;
    lids: [string];
    label_symbols: [string];
    rule_ids: [string];
    store_as_deleted: bool (default = false);
    no_such_folder_action: string; // one of "fail", "fallback_to_inbox", "create"
};

struct TRulesApplier {
    applied_rules: {string -> TAppliedRules}; // by recipientId
    failed_recipients: [string];
};

struct TMailCorpInit {
    corp(required): bool;
};

struct TFirstLine {
    first_line(required): string;
};

struct TThreadHash {
    namespace(required): string;
    value(required): string;
};

struct TThreadLimits {
    days_limit(required): ui32;
    count_limit(required): ui32;
};

struct TThreadInfo {
    hash(required): TThreadHash;
    limits(required): TThreadLimits;
    rule(required): string;
    reference_hashes(required): [string];
    message_ids(required): [string];
    in_reply_to_hash(required): string;
    message_id_hash(required): string;
};

struct TResolvedLabel {
    lid(required): string;
    symbol: string;
};

struct TResolvedFolder {
    fid(required): string;
    name: string;
    type: string;
    type_code: ui32;
};

struct TMdbCommitRecipientResponse {
    uid(required): string;
    status(required): string; // one of "ok", "temp error", "perm error"
    description: string;
    mid: string;
    imap_id: ui32;
    duplicate: bool;
    tid: string;
    folder: TResolvedFolder;
    labels: [TResolvedLabel];
};

struct TMdbCommit {
    status: string (default = "ok"); // one of "ok", "temp error", "perm error"
    responses: {string -> TMdbCommitRecipientResponse};
};

struct TCommitToMdb {}; // just for graph dependencies and edge expressions

struct TVkId {
    id: string;
    stable_sign: ui64;
    delta_pos: ui16;
};

struct TVkInfo {
    ids: [TVkId]; 
};
