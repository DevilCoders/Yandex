namespace NBASSRequest;

struct TForm {
    struct TSlot {
        name        : string (required, != "");
        optional    : bool (required);
        source_text : any;
        type        : string (required, != "");
        value       : any (required);
    };

    name  : string (required, != "");
    slots : [TSlot];
};

struct TMeta {
    struct TDeviceState {
        struct TTimer {
            timer_id          : string (required, cppname = Id);
            currently_playing : bool (required, cppname = IsPlaying);
            paused            : bool (required, cppname = IsPaused);
            start_timestamp   : ui64 (required, cppname = StartAt);
            duration          : ui64 (required, cppname = Duration);
            remaining         : ui64 (required, cppname = Remaining);
        };
        sound_level : i64 (cppname = SoundLevel, default = -1);
        sound_muted : bool (cppname = SoundMuted, default = false);
        music : struct {
            currently_playing : struct {
                track_info : any;
                track_id : string;
            };
            session_id : string;
            playlist_owner : string;
            player : struct {
                pause : bool (default = false);
                timestamp : double;
            };
        };
        video : struct {
            current_screen : string;
            screen_state : any;
            currently_playing : any;
        };
        last_watched : any; // NBassApi::TLastWatchedState
        is_tv_plugged_in : bool (default = false);

        // TODO (@vi002): this is for backward-compatibility and must be removed.
        alarms_state : string (cppname = AlarmsStateObsolete);
        alarm_state (cppname = AlarmsState) : struct {
            icalendar : string (cppname = ICalendar);
            currently_playing : bool;
        };

        timers : struct {
            active_timers : [ TTimer ];
        };
        device_id : string;
        device_model : struct {
            model : string;
            manufacturer : string;
        };
        device_config : struct {
            content_settings : string;
        };

        struct TNavigatorState {
            struct TGeoPoint {
                lat : double (required);
                lon : double (required);
            };

            struct TUserAddress {
                lat : double (required);
                lon : double (required);
                name : string;
                arrival_points : [ TGeoPoint ];
            };

            struct TUserSettings {
                avoid_tolls : bool;
            };

            user_favorites : [ TUserAddress ] (cppname = UserAddresses);
            home : TUserAddress;
            work : TUserAddress;

            user_settings : TUserSettings (cppname = UserSettings);

            current_route : struct {
                points : [ TGeoPoint ];
                distance_to_destination : double;
                arrival_timestamp : i64;
                time_to_destination : i64;
                time_in_traffic_jam : i64;
                distance_in_traffic_jam: double;
            };

            available_voice_ids : [ string ] (cppname = AvailibleVoices);

            // Ad-hoc states for navigator (i.e. "waiting_for_route_confimation")
            states : [ string ] (cppname = States);

        };

        fm_radio : struct {
            region_id : i32;
        };

        installed_apps : any (cppname = InstalledApps);

        navigator: TNavigatorState (cppname = NavigatorState);

        tanker : struct {
            gas_station_id: string;
            fuel_type: string;
            credit_card: bool;
        };
    };

    struct TBiometricsScore {
        score : double (required);
        user_id  : string (required, != "");
    };

    struct TBiometricsScores {
        status : string;
        request_id : string;
        scores : [TBiometricsScore];
    };

    struct TLocation {
        lon : double (required);
        lat : double (required);
        accuracy: double (default = 0);
        accurancy: double (default = 0);
        recency: i64 (default = 0);
    };

    struct TPermission {
        status: bool;
        name: string;
    };

    biometrics_scores : TBiometricsScores;
    client_id : string (default = "telegram/1.0 (none none; none none)");
    client_ip : string (cppname = ClientIP, default = "127.0.0.1");
    cookies : [ string ];
    device_state : TDeviceState (cppname = DeviceState);
    // it is a separate tab if contains anything (size > 0)
    dialog_id  : string;
    epoch : ui64 (required);
    experiments : any;
    filtration_level : ui8 (cppname = FiltrationLevel);
    lang : string (default = "ru-RU");
    location : TLocation;
    region_id : ui32 (cppname = RegionId);
    screen_scale_factor : double (cppname = ScreenScaleFactor, default = 2);
    tld : string (allowed = [ "ru", "ua", "by", "kz", "com.tr", "com" ]);
    tz : string (required, cppname = TimeZone);
    uid : ui64 (cppname = UID);
    user_agent : string (cppname = UserAgent, default = "Mozilla/5.0 (Linux; Android 5.1.1; Nexus 6 Build/LYZ28E) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36");
    utterance : string (cppname = Utterance); // FIXME do it mandatory
    utterance_data : any;
    uuid : string (cppname = UUID, != "");
    yandex_uid : string (cppname = YandexUID);
    device_id : string;
    request_id : string;
    end_of_utterance: bool;
    permissions: [TPermission];
};

struct TSetupRequest {
    forms : [ TForm ];
    meta  : TMeta (required);
};

struct TRequest {
    struct TAction {
        name : string (required, != "");
        data : any;
    };

    struct TBlock {
        type           : string (required, != "");
        suggest_type   : string (cppname = SuggestType);
        attention_type : string (cppname = AttentionType);
        command_type   : string (cppname = CommandType);
        card_template  : string (cppname = CardTemplate);
        form_update    : TForm (cppname = FormUpdate);
        data           : any (required);
    };

    struct TSessionState {
        last_user_info_timestamp : ui64 (cppname = LastUserInfoTimestamp);
    };

    form    : TForm;
    action  : TAction;
    meta    : TMeta (required);
    blocks  : [TBlock];
    session_state : TSessionState (cppname = Session);
    setup_responses : any;
};
