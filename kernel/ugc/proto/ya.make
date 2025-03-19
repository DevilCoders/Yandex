OWNER(
    g:ugc
)

PROTO_LIBRARY()

SRCS(
    achievement.proto
    aggregate.proto
    aspects.proto
    ban_data.proto
    bell_schema.proto
    board_subscription.proto
    communities.proto
    cmnt_init_token.proto
    data_erasure_requests.proto
    deleted_favorite.proto
    device_profile.proto
    device_profile_internal.proto
    favorite_book.proto
    favorite_film.proto
    favorite_game.proto
    favorite_geo.proto
    favorite_goods.proto
    favorite_image.proto
    favorite_link.proto
    favorite_music.proto
    favorite_news.proto
    favorite_organization.proto
    favorite_serial.proto
    favorite_specialist.proto
    favorite_video.proto
    images.proto
    liked_news.proto
    liked_video.proto
    org_chat_review.proto
    pk_settings.proto
    profession.proto
    pushes.proto
    reactions.proto
    sbs_answer.proto
    service_usage.proto
    services_worker.proto
    subscription.proto
    ugcdb_token.proto
    ugc_schema.proto
    unisearch/education.proto
    user_achievement.proto
    user_community.proto
    user_org.proto
    user_profession.proto
    user_profile.proto
    user_profile_internal.proto
    user_splash_screen.proto
    visitor_profile.proto
    visitor_profile_internal.proto
    util.proto
    video_notifications.proto

    afisha/afisha_feedback.proto

    common/author.proto
    common/digest.proto
    common/moderation.proto
    common/property.proto
    common/rating_stats.proto
    common/reaction.proto
    common/ugc_type.proto

    onto/film.proto
    onto/film_review.proto
    onto/series.proto
    onto/series_review.proto
    onto/object.proto

    hqcg/articles_v3.proto
    hqcg/likes.proto
    hqcg/profile.proto

    org/org_answer.proto
    org/org_business_comment.proto
    org/org_digest.proto
    org/org_feedback.proto
    org/org_photo.proto
    org/org_review.proto
    org/org_settings.proto
    org/photo_poll.proto

    site/site_feedback.proto

    direct/direct_comment.proto
    direct/direct_review.proto
    direct/direct_service.proto

    geosearch/business_filter.proto

    sitemap_chunk.proto

    alice_skill_review.proto

    feeder/feeder.proto
)

PEERDIR(
    kernel/ugc/proto/bell
    kernel/ugc/proto/collections
    kernel/ugc/schema/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
