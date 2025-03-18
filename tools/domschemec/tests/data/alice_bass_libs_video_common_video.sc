namespace NBassApi;

struct TLightVideoItem {
    type: string (allowed = ["movie", "tv_show", "tv_show_episode", "video", "tv_stream"]);
    provider_name : string;
    provider_item_id : string;

    // Tor tv_show_episodes this id is an id of the corresponding tv-show season.
    tv_show_season_id : string;

    // For tv_show_episode, this id is an id of the whole tv-show.
    tv_show_item_id : string;

    human_readable_id : string;

    available : bool;
    price_from : ui32;
    episode : ui32;
    season : ui32;
    provider_number : ui32;

    // This field must be used for UI only.
    // TODO (@vi002): extract UI-only fields into an individual entity.
    unauthorized : bool;
};

struct TSeasonsItem {
    number : ui64;
};

struct TVideoItem : TLightVideoItem {
    cover_url_2x3 : string;
    cover_url_16x9 : string;
    thumbnail_url_2x3_small : string;
    thumbnail_url_16x9 : string;
    thumbnail_url_16x9_small : string;
    name : string;
    description : string;
    duration : ui32;
    genre : string;
    rating : double;
    review_available : bool;
    progress : double;
    seasons_count : ui32;
    episodes_count : ui32;

    struct TSeason {
        id : string;
        episodes : [TVideoItem];
        provider_number : ui64;
        soon : bool;
    };

    kinopoisk_info : struct {
        seasons : [TSeason];
    };

    release_year : ui32;
    directors : string;
    actors : string;
    source_host : string;
    view_count : ui64;
    play_uri : string;

    relevance : ui64;
    relevance_prediction : double;

    misc_ids : struct {
        kinopoisk : string;
        kinopoisk_uuid : string;
        imdb : string;
    };

    provider_info : [TLightVideoItem];
    availability_request : any;

    next_items : [TVideoItem];
    previous_items : [TVideoItem];

    // Needed only when type is tv_show
    seasons : [TSeasonsItem];

    // Age restriction on the video item.
    min_age : ui32;

    // Needed only when type is tv_stream
    tv_stream_info : struct {
        channel_type : string;
        tv_episode_name : string;
    };
    // TODO remove when possible (see ASSISTANT-2850)
    channel_type : string;
    tv_episode_name : string;

    soon : bool;
};

struct TVideoGallery {
    question : string;
    answer : string;
    background_16x9 : string;

    items : [TVideoItem];
    tv_show_item : TVideoItem;
    season : ui32;
};

// Commands

struct TShowPayScreenCommandData {
    item : TVideoItem;

    // Needed only when item is a tv_show_episode.
    tv_show_item : TVideoItem;
};

struct TRequestContentPayload {
    item : TVideoItem;

    // Needed only when item is a tv_show.
    season_index : ui32;
};

struct TPlayVideoCommandData {
    uri : string;

    // Needed to pass any aux info from video provider to video
    // player.
    payload : string;

    session_token : string;
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
    start_at : double;
};

struct TPlayVideoActionData {
    item : TVideoItem;
    tv_show_item : TVideoItem;
};

struct TShowVideoDescriptionCommandData {
    item : TVideoItem;
    tv_show_item : TVideoItem;
};

// Device state

struct TScreenState {
    current_screen : string;
};

struct TDescriptionScreenState : TScreenState {
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
};

struct TVideoProgress {
    played : double;
    duration : double;
};

struct TVideoCurrentlyPlaying : TScreenState {
    paused : bool;
    progress : TVideoProgress;
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
};

struct TWatchedVideoItem : TLightVideoItem {
    progress : TVideoProgress;
    timestamp : i64;
};
struct TWatchedTvShowItem : TLightVideoItem {
    item : TWatchedVideoItem;
    tv_show_item : TWatchedVideoItem;
};

struct TLastWatchedState {
    tv_shows : [TWatchedTvShowItem];
    movies : [TWatchedVideoItem];
    videos : [TWatchedVideoItem];
};

// Billing API

struct TPricingOption {
    userPrice : ui32 (cppname = UserPrice);
    provider : string;
};

struct TQuasarBillingPricingOptions {
    available : bool;

    pricingOptions : [TPricingOption] (cppname = PricingOptions);
};

// Kinopoisk data

struct TKinopoiskContentItem {
    film_kp_id : string;
    film_uuid : string;
};
