#include "Arduino.h"
#include "esp32_smartdisplay.h"
#include "WiFiClientSecure.h"
#include "WiFi.h"
#include "NostrEvent.h"
#include "NostrRelayManager.h"
#include "time.h"


/////////////////////////////////////////////////////////////

/**********************
 *      SETUP
 **********************/
char * SSID = "MIWIFI_2G_y36Y";
char * wifi_password = "34sf36crhwnp";
char const *nsecHex = "81468c8cb1d103a211482589d547aefa70005052ca30ddead9e0659a72629448";
char const *npubHex = "0fa58617cd7be70fd5048f33683fb97c78d11ec32b266b1f25098de37a6caa63";

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;


#define EVENT_KIND_PM 4;
#define EVENT_KIND_RESUME 66;
#define EVENT_KIND_STALL 30017;
#define EVENT_KIND_PRODUCT 30018;

/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;

static lv_obj_t * tab_view;

static const lv_font_t * font_large;
static const lv_font_t * font_normal;

/**********************
 *  NOSTR
 **********************/
NostrEvent nostr;
NostrRelayManager nostrRelayManager;
NostrQueueProcessor nostrQueue;

/////////////////////////////////////////////////////////////
void logMemory(){
  log_d("Total heap: %d", ESP.getHeapSize());
  log_d("Free heap: %d", ESP.getFreeHeap());
}



void setup_app()
{
    // Clear screen
    lv_obj_clean(lv_scr_act());

    if(LV_HOR_RES <= 320) disp_size = DISP_SMALL;
    else if(LV_HOR_RES < 720) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

logMemory();

    lv_coord_t tab_h;
    if(disp_size == DISP_LARGE) {
        tab_h = 70;
#if LV_FONT_MONTSERRAT_24
        font_large     = &lv_font_montserrat_24;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
        font_normal    = &lv_font_montserrat_16;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else if(disp_size == DISP_MEDIUM) {
        tab_h = 45;
#if LV_FONT_MONTSERRAT_20
        font_large     = &lv_font_montserrat_20;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_14
        font_normal    = &lv_font_montserrat_14;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_14 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else {   /* disp_size == DISP_SMALL */
        tab_h = 45;
#if LV_FONT_MONTSERRAT_18
        font_large     = &lv_font_montserrat_18;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
        font_normal    = &lv_font_montserrat_12;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
#endif
logMemory();
    tab_view = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

    lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

    lv_obj_t * tab_orders = lv_tabview_add_tab(tab_view, "Orders");
    lv_obj_t * tab_stock = lv_tabview_add_tab(tab_view, "Stock");
    lv_obj_t * tab_setup = lv_tabview_add_tab(tab_view, "Setup");

    /*Add content to the tabs*/
    lv_obj_t * label = lv_label_create(tab_orders);
    lv_label_set_text(label, "This the first tab\n\n"
                      "If the content\n"
                      "of a tab\n"
                      "becomes too\n"
                      "longer\n"
                      "than the\n"
                      "container\n"
                      "then it\n"
                      "automatically\n"
                      "becomes\n"
                      "scrollable.\n"
                      "\n"
                      "\n"
                      "\n"
                      "Can you see it?\n"
                      "please see it\n"
                      "Thanks\n"
                      "for\n"
                      "collaborate.");

    label = lv_label_create(tab_stock);
    lv_label_set_text(label, "Second tab");

    label = lv_label_create(tab_setup);
    lv_label_set_text(label, "WIFI\n Status: " + WiFi.status());

    lv_obj_scroll_to_view_recursive(label, LV_ANIM_ON);
logMemory();
}

void initializeTime() {
    Serial.println("Connecting NTP...");

    // NTP server to request epoch time
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 1;
    const int   daylightOffset_sec = 7200;

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
logMemory();
    Serial.println("NTP configured");
}


void initializeWifi() {
    WiFi.begin(SSID, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
logMemory();
    Serial.println("Connected to WiFi");
}



unsigned long getUnixTimestamp() {
  time_t now;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 0;
  } else {
    Serial.println("Got timestamp of " + String(now));
  }
  time(&now);
  return now;
}

void okEvent(const std::string& key, const char* payload) {
    Serial.println("OK event");
    Serial.println("payload is: ");
    Serial.println(payload);
}

void nip01Event(const std::string& key, const char* payload) {
    Serial.println("NIP01 event");
    Serial.println("payload is: ");
    Serial.println(payload);
}

void nip04Event(const std::string& key, const char* payload) {
    Serial.println("NIP04 event");
    String dmMessage = nostr.decryptDm(nsecHex, payload);
    Serial.println("message is: ");
    Serial.println(dmMessage);
}

void initializeNostr() {
    const char *const relays[] = {
      "relay.damus.io",
      "nostr.mom",
      "relay.nostr.bg",
      "nos.lol",
      "nostr.bitcoiner.social",
      "nostr.wine",
      "eden.nostr.land",
      "relay.orangepill.dev",
    };
logMemory();
    int relayCount = sizeof(relays) / sizeof(relays[0]);
    
    nostr.setLogging(true);
    nostrRelayManager.setMinRelaysAndTimeout(2,10000);

    // Set some event specific callbacks here
    nostrRelayManager.setEventCallback("ok", okEvent);
    nostrRelayManager.setEventCallback("nip01", nip01Event);
    nostrRelayManager.setEventCallback("nip04", nip04Event);
    nostrRelayManager.connect();

logMemory();
//    String subscriptionString = "[\"REQ\", \"" + nostrRelayManager.getNewSubscriptionId() + "\", {\"authors\": [\"27c3d086c54a1862d38bf4f0ffb7c4f1be21a037b8ce3a964b8ed34bb340914f\"], \"kinds\": ["+EVENT_KIND_PRODUCT+"], \"limit\": 20}]";
//    nostrRelayManager.enqueueMessage(subscriptionString.c_str());


    //subscriptionString = nostr.getEncryptedDm(nsecHex, npubHex, testRecipientPubKeyHex, timestamp, "Running NIP04!");
    //nostrRelayManager.enqueueMessage(subscriptionString.c_str());

}

void setup()
{
    logMemory();
    Serial.begin(115200);
    Serial.println("   --------------- Starting app...");

    #if LV_USE_BUILTIN_MALLOC && LV_MEM_SIZE < (38ul * 1024ul)
        #error Insufficient memory. Please set LV_MEM_SIZE to at least 38KB (38ul * 1024ul).  48KB is recommended.
    #endif

    smartdisplay_init();
    setup_app();
    initializeWifi();
    initializeTime();
    logMemory();
    initializeNostr();
}

void loop()
{
//    lv_timer_handler();
    nostrRelayManager.loop();
    nostrRelayManager.broadcastEvents();
}