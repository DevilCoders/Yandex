namespace NHQCG::NApi;

// Краткая информация об авторе статей (не о себе)
struct TAuthor {
    puid        : string (required, != "", cppname = PassportUID);
    login       : string (required, != "");
    display_name: string;
    avatar      : string;
    karma       : ui64;
};

// статистика показов статьи
struct TArticleShowStats {
    struct TEntry {
        label : string (required, != "");
        value : ui32   (required);
        icon  : string;
    };
    entries : [TEntry];
};

// текущий стейт статьи
struct TArticleState {
    draft           : bool (required); // есть изменения по сравнению с опубликованной версий или опубликованной версии нет
    published_at    : ui64; // опубликована с ...
    turbo_url       : string; // для опубликованной статьи передаём её url
    publishing_at   : ui64; // начала публикации (ещё не завершено)
    unpublishing_at : ui64; // начало снятия с публикации (ещё не завершено)
    moderating_at   : ui64; // время отправки на модерацию
    description     : string; // более детальное описание состояния
};

// мета-информация о статье, без контента
struct TArticleInfo {
    id          : string (required);
    title       : string (required);
    abstract    : string;
    score       : ui64 (required);
    tags        : [string];

    turbo_url   : string; // only for Public article

    state       : TArticleState (required);

    created_at  : ui64 (required);
    modified_at : ui64;
    published_at: ui64; // required for public articles

    show_stats  : TArticleShowStats;
    visit_stats  : TArticleShowStats;

    struct TMoveHistory {
        timestamp : ui64 (required);
        from      : string (required);
        message   : string (required);
    };
    move_history : [TMoveHistory];
};

// мета-информация для List-операций, аналог found в поиске
// filter/sort/offset/limit - cgi-параметры исходного запроса, необходимо для правильной пагинации
struct TListMeta {
    total   : ui32 (required);
    filter  : string; // cgi parameter 'filter' if exists
    sort    : string; // cgi parameter 'sort' if exists
    offset  : ui32;   // cgi parameter 'offset' if exists
    limit   : ui32;   // cgi parameter 'limit' if exists
};

// ответ на запрос списка статей какого-то пользователя (не своих)
struct TListArticlesResponse {
    author      : TAuthor (required);
    articles    : [TArticleInfo] (required);
    meta        : TListMeta (required);
};

// ответ на запрос своих статей
struct TListOwnArticlesResponse {
    articles    : [TArticleInfo] (required);
    meta        : TListMeta (required);
};

// ответ на запрос загрузки своей статьи
struct TLoadOwnArticleResponse {
    article_info    : TArticleInfo (required);
    content         : string (required);
};

// запрос на создание статьи
struct TCreateArticleRequest {
    title       : string;
    abstract    : string;
    tags        : [string];
    content     : string (required, != "");

    (validate) {
        if (!helper->CheckNotEmpty(Content()))
            AddError("\"content\" is empty");
    };
};

// ответ на запрос о создании статьи
struct TCreateArticleResponse {
    id    : string (required, != "");
    state : TArticleState (required);

    (validate) {
        if (!helper->CheckNotEmpty(Id()))
            AddError("\"id\" is empty");
    };
};

// запрос на апдейт статьи
struct TUpdateArticleRequest {
    id          : string (required, != "");
    title       : string;
    abstract    : string;
    tags        : [string];
    content     : string;

    (validate) {
        if (!helper->ValidateArticleId(Id()))
            AddError("\"id\" validation failed");
    };
};

// ответ на запрос обновления статьи
struct TUpdateArticleResponse {
    id    : string (required, != "");
    state : TArticleState (required);
};

// контакт пользователя
struct TUserContact {
    type        : string (required, allowed = ["email", "phone", "telegram", "skype", "facebook", "vk", "ok", "twitter"]);
    value       : string (required);
    description : string;
};

// запрос домашней страницы
struct THomePageResponse {
    author      : TAuthor (required);

    about       : string (required);
    contacts    : [TUserContact] (required);

    public_articles_count : ui32;
    draft_articles_count  : ui32; // not empty only for self

    favorites_themes : [string];
    top_articles     : [TArticleInfo];  // public only
};

struct TPublishArticleRequest {
    turbo_json : any; // json
    turbo_host : string; // talk.yandex.ru, health.yandex.ru, realty.yandex.ru, etc

    struct TModeration {
        contest_id : string; // id конкурса
    };
    moderation : TModeration;

    struct TInternal {
        origin_url : string;
        wait       : bool; // wait until article will be published
    };
    internal : TInternal;
};

// ответ на публикацию статьи
struct TPublishArticleResponse {
    id    : string (required, != "");
    state : TArticleState (required);
};

// ответ на снятие статьи с публикации
struct TUnpublishArticleResponse {
    id    : string (required, != "");
    state : TArticleState (required);
};

// ответ на запрос лайков по статье
struct TArticleLikesResponse {
    score : i64 (required); // текущий рейтинг статьи (оно же "количество лайков")
    my    : string (required, allowed = ["none", "like", "dislike"]); // моя оценка
};

// запрос на перемещение статьи другому пользователю
struct TMoveArticleRequest {
    to      : string (required); // recipient login
    message : string (required);
};

// запрос на изменение профиля
struct TUpdateProfileRequest {
    about    : string;
    contacts : [TUserContact];
};
