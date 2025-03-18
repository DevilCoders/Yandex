namespace NFrontEndApi;

struct TDraftJsToTurboJsonRequest {
    articleId   : string (required, cppname = ArticleId);
    title       : string (required, cppname = ArticleTitle);
    login       : string (required, cppname = AuthorLogin);
    name        : string (required, cppname = DisplayName);
    encodedPuid : string (required, cppname = EncodedPassportUid);
    publishedAt : ui64   (required, cppname = PublishedAt);
    content     : any    (required, cppname = DraftJs);
};
