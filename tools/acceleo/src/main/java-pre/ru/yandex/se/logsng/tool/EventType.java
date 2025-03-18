package ru.yandex.se.logsng.tool;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EModelElement;

import java.util.HashSet;

/**
 * Created by astelmak on 15.08.16.
 */
public class EventType {
    private static final HashSet<String> oldStyleTypes = new HashSet<>();

    static {
        oldStyleTypes.add("WEATHER:WEATHER_ACCESS_EVENT");
        oldStyleTypes.add("REPORT:WEB_REPORT_SEARCH_REQANS_EVENT");
        oldStyleTypes.add("REPORT:ACCESS_EVENT");
        oldStyleTypes.add("ATOM:ATOM_FRONT_REQUEST_EVENT");
        oldStyleTypes.add("REPORT:ATOM_REQUEST_EVENT");
        oldStyleTypes.add("LAYOUT:BLOCKSTAT_EVENT");
        oldStyleTypes.add("LAYOUT:CLIENT_EVENT");
        oldStyleTypes.add("REPORT:CREATION_CLIENT_EVENT_ERROR");
        oldStyleTypes.add("COMMON:ERROR_EVENT");
        oldStyleTypes.add("MOBILE:EXPERIMENT_CONFIG_APPLIED_EVENT");
        oldStyleTypes.add("MOBILE:EXPERIMENT_CONFIG_RECEIVED_EVENT");
        oldStyleTypes.add("MOBILE:EXPERIMENT_EVENT");
        oldStyleTypes.add("MOBILE:FAB_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:FAB_VISIBILITY_EVENT");
        oldStyleTypes.add("MOBILE:FIRST_WIDGET_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:GEO_STATE_CHANGED_DEBUG_EVENT");
        oldStyleTypes.add("MOBILE:GEO_STATE_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:GPS_SATELLITE_INFO_EVENT");
        oldStyleTypes.add("MOBILE:GPS_STATE_EVENT");
        oldStyleTypes.add("MOBILE:HEADSET_STATE_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:HEARTBEAT_EVENT");
        oldStyleTypes.add("MOBILE:HISTORY_CLEAR_EVENT");
        oldStyleTypes.add("MOBILE:HW_BUTTON_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:INSTANT_SEARCH_ACTIVATED_EVENT");
        oldStyleTypes.add("MOBILE:INSTANT_SEARCH_SHOWED_EVENT");
        oldStyleTypes.add("MOBILE:INTERNAL_MONITORING_STATS_EVENT");
        oldStyleTypes.add("MOBILE:ITEM_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:JS_API_EVENT");
        oldStyleTypes.add("MOBILE:JS_API_TECH_EVENT");
        oldStyleTypes.add("MOBILE:KL_LITE_LOCATION_EVENT");
        oldStyleTypes.add("MOBILE:KL_LITE_STATISTICS_EVENT");
        oldStyleTypes.add("MOBILE:LOCK_SCREEN_NOTIFICATION_OPEN_NOTIFICATION_SETTINGS_EVENT");
        oldStyleTypes.add("MOBILE:LOCK_SCREEN_NOTIFICATION_OPEN_SECURITY_SETTINGS_EVENT");
        oldStyleTypes.add("MOBILE:LOCKSCREEN_SETTING_DIALOG_EVENT");
        oldStyleTypes.add("MOBILE:LOGGER_LIB_VERSION_EVENT");
        oldStyleTypes.add("MOBILE:MENU_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:METRIKA_ID_UPDATED_EVENT");
        oldStyleTypes.add("MOBILE:MOBILE_DATA_STATE_EVENT");
        oldStyleTypes.add("REPORT:MOBILE_REPORT_REQANS_EVENT");
        oldStyleTypes.add("MOBILE:MOBILE_SEARCH_REQUEST_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CARD_CONTENT_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CARD_CONTENT_HORIZONTAL_SCROLL_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CITY_SETTINGS_CHANGE_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CITY_SETTINGS_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CONTENT_SCROLLED_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_CONTENT_SHOWED_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_ERROR_MESSAGE_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_PTR_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_REQUEST_STATS_EVENT");
        oldStyleTypes.add("MOBILE:MORDA_SESSION_EVENT");
        oldStyleTypes.add("MOBILE:NETWORK_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:NETWORK_REQUEST_EVENT");
        oldStyleTypes.add("MOBILE:OFFLINE_DICTIONARY_DELETED_EVENT");
        oldStyleTypes.add("MOBILE:OFFLINE_SEARCH_LOSS_EVENT");
        oldStyleTypes.add("MOBILE:OFFLINE_SETTINGS_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:OLD_METRIKA_UNTYPED_EVENT");
        oldStyleTypes.add("MOBILE:OMNIBOX_SWIPE_EVENT");
        oldStyleTypes.add("MOBILE:OMNIBOX_VIEW_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:ONLINE_SERP_READY_EVENT");
        oldStyleTypes.add("MOBILE:OPEN_LINK_FROM_WEB_VIEW_EVENT");
        oldStyleTypes.add("MOBILE:OPEN_LINKS_IN_YA_BRO_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:OPEN_LINKS_IN_YA_BRO_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:PREFETCH_COMMIT_EVENT");
        oldStyleTypes.add("MOBILE:PUSH_CLICKED_EVENT");
        oldStyleTypes.add("MOBILE:PUSH_DISMISSED_EVENT");
        oldStyleTypes.add("MOBILE:PUSH_RECEIVED_EVENT");
        oldStyleTypes.add("MOBILE:PUSH_TOKEN_EVENT");
        oldStyleTypes.add("MOBILE:QUERY_OMNIBOX_EVENT");
        oldStyleTypes.add("MOBILE:RAW_LBS_DATA_EVENT");
        oldStyleTypes.add("REPORT:REPORT_ERROR_EVENT");
        oldStyleTypes.add("REPORT:REPORT_REQUEST_PROFILE_EVENT");
        oldStyleTypes.add("MOBILE:REQUEST_COMPLETED_EVENT");
        oldStyleTypes.add("MOBILE:REQUEST_COMPLITED_EVENT");
        oldStyleTypes.add("MOBILE:REQUEST_STARTED_EVENT");
        oldStyleTypes.add("MOBILE:RESOURCE_USAGE_REPORT_EVENT");
        oldStyleTypes.add("MOBILE:SCREEN_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:SCREEN_ORIENTATION_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:SCREEN_STATE_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:SEARCH_APP_INCIDENT_EVENT");
        oldStyleTypes.add("MOBILE:SEARCHAPP_SETTINGS_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:SEARCH_APP_SETTINGS_EVENT");
        oldStyleTypes.add("MOBILE:SEARCH_APP_SETTINGS_EVENT_V450");
        oldStyleTypes.add("MOBILE:SEARCH_EVENT");
        oldStyleTypes.add("MOBILE:SEARCH_REQUEST_STATS_EVENT");
        oldStyleTypes.add("MOBILE:SEARCH_VERTICAL_SWITCH_EVENT");
        oldStyleTypes.add("MOBILE:SENSOR_INFO_EVENT");
        oldStyleTypes.add("MOBILE:SERP_CLICKED_EVENT");
        oldStyleTypes.add("MOBILE:SERP_INSTANT_EVENT");
        oldStyleTypes.add("MOBILE:SERP_SHOWN_EVENT");
        oldStyleTypes.add("REPORT:SERVER_USER_BIRTH_EVENT");
        oldStyleTypes.add("MOBILE:SIGNAL_STRENGTH_EVENT");
        oldStyleTypes.add("MOBILE:SLICE_ROLLBACK_ITEM_EVENT");
        oldStyleTypes.add("MOBILE:SPEECH_KIT_BUTTON_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:SPEECH_KIT_RESULT_SELECTED_EVENT");
        oldStyleTypes.add("MOBILE:SPEECHKIT_RESULTS_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:SPEECH_KIT_SESSION_EVENT");
        oldStyleTypes.add("MOBILE:STARTUP_REQUEST_STATS_EVENT");
        oldStyleTypes.add("MOBILE:SUGGEST_EVENT");
        oldStyleTypes.add("MOBILE:SYSTEM_DEFAULT_BROWSER_EVENT");
        oldStyleTypes.add("MOBILE:TECH_INFO_EVENT");
        oldStyleTypes.add("REPORT:TEMPLATE_REQUEST_PROFILE_EVENT");
        oldStyleTypes.add("LAYOUT:TESTSAFECLICKEVENT");
        oldStyleTypes.add("MOBILE:TIME_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:TIMING_EVENT");
        oldStyleTypes.add("MOBILE:TRAFFIC_CHART_EVENT");
        oldStyleTypes.add("MOBILE:TRASH_EVENT");
        oldStyleTypes.add("MOBILE:UI_ACTION_EVENT");
        oldStyleTypes.add("MOBILE:UI_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:UNSENT_EVENTS_STATS_EVENT");
        oldStyleTypes.add("MOBILE:USER_APPLICATION_BANNED_EVENT");
        oldStyleTypes.add("MOBILE:USER_BIRTH_EVENT");
        oldStyleTypes.add("MOBILE:USER_DEVICE_FONT_SCALE_EVENT");
        oldStyleTypes.add("MOBILE:USER_LOCALE_EVENT");
        oldStyleTypes.add("MOBILE:USER_PHONE_CONTACT_INFO_EVENT");
        oldStyleTypes.add("MOBILE:VIEWPORT_BLOCK_CLICKED_EVENT");
        oldStyleTypes.add("MOBILE:VIEWPORT_BLOCK_CLICKED_EVENT_FROM_INFORMER_EVENT");
        oldStyleTypes.add("MOBILE:VIEWPORT_CARD_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:VIEWPORT_EVENT");
        oldStyleTypes.add("MOBILE:VOICE_ANSWER_STOP_EVENT");
        oldStyleTypes.add("MOBILE:WEBVIEW_MORDA_CARD_EVENT");
        oldStyleTypes.add("MOBILE:WEBVIEW_VSCROLL_EVENT");
        oldStyleTypes.add("MOBILE:WELCOME_SCREEN_SHOWED_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_APP_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_APP_SHORTCUT_CLICKED_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_APP_SHORTCUT_ITEM_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_CARD_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_CARD_STUB_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_HEARTBEAT_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_LIFECYCLE_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_REGION_APP_SHORTCUT_ITEM_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_VIEW_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:WIDGET_VIEWPORT_CONFIG_ITEM_EVENT");
        oldStyleTypes.add("MOBILE:WIFI_POINT_INFO_EVENT");
        oldStyleTypes.add("MOBILE:WIN_DEVICE_INFO_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:YELLOW_SKIN_BACK_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:YELLOW_SKIN_MOCK_BUTTON_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:YELLOW_SKIN_UI_BACK_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:ACCOUNT_INFO_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:ACCOUNT_LOGIN_EVENT");
        oldStyleTypes.add("MOBILE:AJAX_CALLBACK_EVENT");
        oldStyleTypes.add("MOBILE:ALL_SERVICES_NATIVE_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:ALL_SERVICES_NATIVE_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:ALREADY_INSTALLED_RECOMMENDED_APP");
        oldStyleTypes.add("MOBILE:ALREADY_INSTALLED_RECOMMENDED_APP_EVENT");
        oldStyleTypes.add("MOBILE:ANDROID_DEVICE_INFO_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:APP_BIND_EVENT");
        oldStyleTypes.add("MOBILE:APP_LAUNCH_STATUS_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_CLIENT_ID_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_INSTALLED_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_LIST_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_PERFORMANCE_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_PRESENT_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_REMOVED_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_STARTED_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_START_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_STOPPED_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_SWITCHED_TO_BACKGROUND_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_SWITCHED_TO_FOREGROUND_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_UPDATED_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_UPDATE_SUGGEST_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:APPLICATION_UPDATE_SUGGEST_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:AVAILABLE_ID_EVENT");
        oldStyleTypes.add("MOBILE:BACK_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:BATTERY_EVENT");
        oldStyleTypes.add("MOBILE:BATTERY_LEVEL_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:BATTERY_POWER_CONNECTED_EVENT");
        oldStyleTypes.add("MOBILE:BATTERY_POWER_DISCONNECTED_EVENT");
        oldStyleTypes.add("MOBILE:BATTERY_POWER_STATUS_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:BLOCK_STAT_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_BACK_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_COOKIE_HIJACKER_REDIRECT_STATUS_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_EXIT_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_IS_OPENED_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_MENU_ITEM_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:BROWSER_MENU_PRESSED_EVENT");
        oldStyleTypes.add("MOBILE:CALL_STATE_INFO_EVENT");
        oldStyleTypes.add("MOBILE:CARD_BLACKLIST_RESULT_EVENT");
        oldStyleTypes.add("MOBILE:CARD_BLACKLIST_SHOWN_EVENT");
        oldStyleTypes.add("MOBILE:CELL_INFO_CDMA_EVENT");
        oldStyleTypes.add("MOBILE:CELL_INFO_GSM_EVENT");
        oldStyleTypes.add("MOBILE:CELL_INFO_LTE_EVENT");
        oldStyleTypes.add("MOBILE:CELL_INFO_WCDMA_EVENT");
        oldStyleTypes.add("MOBILE:CELL_SERVICE_STATE_EVENT");
        oldStyleTypes.add("MOBILE:CITY_SETTINGS_CLICK_EVENT");
        oldStyleTypes.add("MOBILE:CLEAN_APPLICATION_STARTED_EVENT");
        oldStyleTypes.add("MOBILE:CLIENT_BLOCK_STAT_EVENT");
        oldStyleTypes.add("MOBILE:CLIENT_JS_ERROR");
        oldStyleTypes.add("MOBILE:CLIENT_OFFLINE_SEARCH_RESPONSE_EVENT");
        oldStyleTypes.add("MOBILE:CLIENT_SEARCH_REQUEST_EVENT");
        oldStyleTypes.add("MOBILE:CLIENT_SEARCH_RESPONSE_EVENT");
        oldStyleTypes.add("MOBILE:CLIENT_SERVER_TIME_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:CREATION_CLIENT_EVENT_ERROR_EVENT");
        oldStyleTypes.add("MOBILE:CURRENT_CDMA_CELL_EVENT");
        oldStyleTypes.add("MOBILE:CURRENT_GSM_CELL_EVENT");
        oldStyleTypes.add("MOBILE:DATA_REFRESH_WIDGET_EVENT");
        oldStyleTypes.add("MOBILE:DELIVERY_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_BOOT_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_CHARGING_STATE_CHANGED_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_GPS_AVAILABILITY_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_INFO_EVENT");
        oldStyleTypes.add("MOBILE:DEVICE_INFO_SYNC_EVENT");
        oldStyleTypes.add("MOBILE:DRAWER_ITEM_CLICK_EVENT");
        oldStyleTypes.add("ATOM:ATOM_FRONT_ANSWER_EVENT");
    }

    public static boolean isOldStyleEnumNaming(String module, String name) {
        String key = ((module == null || module.trim().isEmpty() ? "" : (module + ':')) + name).toUpperCase();
        return oldStyleTypes.contains(key);
    }

    public static boolean isOldStyleEnumNaming(EModelElement element) {
        return isOldStyleEnumNaming(getModuleName(element), getXSDTypeName(element));
    }

    private static String getModuleName(EModelElement el) {
        EAnnotation an = el.getEAnnotation("scarab:builtin:module");
        if (an == null)
            return null;
        return an.getDetails().get("appinfo");
    }

    private static String getXSDTypeName(EModelElement el) {
        return el.getEAnnotation("http:///org/eclipse/emf/ecore/util/ExtendedMetaData").getDetails().get("name");
    }

    public static String getEventTypeEnumName(String module, String name) {
        return ((isOldStyleEnumNaming(module, name) ? "" : (module + '_')) + name).toUpperCase();
    }
}
