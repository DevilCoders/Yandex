LIBRARY()

OWNER(g:alice)

COMPILE_NLG(
    common/cards.nlg
    common/json_macro.nlg
    common/macros_ar.nlg
    common/macros_en.nlg
    common/macros_ru.nlg

    video/video__common_ar.nlg
    video/video__common_en.nlg
    video/video__common_ru.nlg
    video/authorize_video_provider_ar.nlg
    video/authorize_video_provider_en.nlg
    video/authorize_video_provider_ru.nlg
    video/browser_video_gallery_ar.nlg
    video/browser_video_gallery_en.nlg
    video/browser_video_gallery_ru.nlg
    video/change_track_ar.nlg
    video/change_track_en.nlg
    video/change_track_ru.nlg
    video/change_track_hardcoded_ar.nlg
    video/change_track_hardcoded_en.nlg
    video/change_track_hardcoded_ru.nlg
    video/finished_ar.nlg
    video/finished_en.nlg
    video/finished_ru.nlg
    video/goto_video_screen_ar.nlg
    video/goto_video_screen_en.nlg
    video/goto_video_screen_ru.nlg
    video/open_current_video_ar.nlg
    video/open_current_video_en.nlg
    video/open_current_video_ru.nlg
    video/open_current_video__callback_ar.nlg
    video/open_current_video__callback_en.nlg
    video/open_current_video__callback_ru.nlg
    video/open_current_trailer_ar.nlg
    video/open_current_trailer_en.nlg
    video/open_current_trailer_ru.nlg
    video/payment_confirmed_ar.nlg
    video/payment_confirmed_en.nlg
    video/payment_confirmed_ru.nlg
    video/payment_confirmed__callback_ar.nlg
    video/payment_confirmed__callback_en.nlg
    video/payment_confirmed__callback_ru.nlg
    video/select_video_from_gallery_ar.nlg
    video/select_video_from_gallery_en.nlg
    video/select_video_from_gallery_ru.nlg
    video/show_video_settings_ar.nlg
    video/show_video_settings_en.nlg
    video/show_video_settings_ru.nlg
    video/skip_video_fragment_ar.nlg
    video/skip_video_fragment_en.nlg
    video/skip_video_fragment_ru.nlg
    video/video_how_long_ar.nlg
    video/video_how_long_en.nlg
    video/video_how_long_ru.nlg
    video/video_play_ar.nlg
    video/video_play_en.nlg
    video/video_play_ru.nlg
)

END()
