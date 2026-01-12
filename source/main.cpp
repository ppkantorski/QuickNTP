#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <string>
#include <vector>
#include <climits>

#include "ntp-client.hpp"
#include "ini_funcs.hpp"

TimeServiceType __nx_time_service_type = TimeServiceType_System;

const char* iniLocations[] = {
    "/config/quickntp.ini",
    "/config/quickntp/config.ini",
    "/switch/.overlays/quickntp.ini",
};
const char* iniSection = "Servers";

const char* defaultServerAddress = "pool.ntp.org";
const char* defaultServerName = "NTP Pool Main";

class NtpGui : public tsl::Gui {
private:
    int currentServer = 0;
    bool blockFlag = false;
    std::vector<std::string> serverAddresses;
    std::vector<std::string> serverNames;

    std::string getCurrentServerAddress() {
        return serverAddresses[currentServer];
    }

    bool setNetworkSystemClock(time_t time) {
        Result rs = timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)time);
        return R_SUCCEEDED(rs);
    }

    void setTime() {
        std::string srv = getCurrentServerAddress();
        NTPClient* client = new NTPClient(srv.c_str());

        time_t ntpTime = client->getTime();
        
        if (ntpTime != 0) {
            if (setNetworkSystemClock(ntpTime)) {
                if (tsl::notification)
                    tsl::notification->showNow(ult::NOTIFY_HEADER+"Synced with " + srv, 22);
            } else {
                if (tsl::notification)
                    tsl::notification->showNow(ult::NOTIFY_HEADER+"Unable to set network clock", 22);
            }
        } else {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Error: Failed to get NTP time", 22);
        }

        delete client;
    }

    void setNetworkTimeAsUser() {
        // Check internet connectivity first
        NifmInternetConnectionStatus connectionStatus;
        Result nifmResult = nifmGetInternetConnectionStatus(nullptr, nullptr, &connectionStatus);
        if (R_FAILED(nifmResult) || connectionStatus != NifmInternetConnectionStatus_Connected) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Unable to set network clock", 22);
            return;
        }
    
        time_t userTime, netTime;
    
        Result rs = timeGetCurrentTime(TimeType_UserSystemClock, (u64*)&userTime);
        if (R_FAILED(rs)) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"GetTimeUser " + std::to_string(rs), 22);
            return;
        }
    
        // Validate that userTime is reasonable (not 0 or negative)
        if (userTime <= 0) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Invalid user time", 22);
            return;
        }
    
        // Check if user time appears to be uninitialized (before year 2017, when Switch was released)
        // Unix timestamp for Jan 1, 2017 is 1483228800
        if (userTime < 1483228800) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"User time not set", 22);
            return;
        }
    
        std::string usr = "User time set!";
        std::string gr8 = "";
        rs = timeGetCurrentTime(TimeType_NetworkSystemClock, (u64*)&netTime);
        if (R_SUCCEEDED(rs) && netTime < userTime) {
            gr8 = " Great Scott!";
        }
    
        Result setResult = timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)userTime);
        if (R_SUCCEEDED(setResult)) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+usr + gr8, 22);
        } else {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Unable to set network clock", 22);
        }
    }

    void getOffset() {
        time_t currentTime;
        Result rs = timeGetCurrentTime(TimeType_NetworkSystemClock, (u64*)&currentTime);
        if (R_FAILED(rs)) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"GetTimeNetwork " + std::to_string(rs), 22);
            return;
        }

        std::string srv = getCurrentServerAddress();
        NTPClient* client = new NTPClient(srv.c_str());

        time_t ntpTimeOffset = client->getTimeOffset(currentTime);
        
        if (ntpTimeOffset != LLONG_MIN) {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Offset: " + std::to_string(ntpTimeOffset) + "s", 22);
        } else {
            if (tsl::notification)
                tsl::notification->showNow(ult::NOTIFY_HEADER+"Error: Failed to get offset", 22);
        }

        delete client;
    }

    bool operationBlock(std::function<void()> fn) {
        if (!blockFlag) {
            blockFlag = true;
            fn();
            blockFlag = false;
        }
        return !blockFlag;
    }

    std::function<std::function<bool(u64 keys)>(int key)> syncListener = [this](int key) {
        return [=, this](u64 keys) {
            if (keys & key) {
                return operationBlock([&]() {
                    setTime();
                });
            }
            return false;
        };
    };

    std::function<std::function<bool(u64 keys)>(int key)> offsetListener = [this](int key) {
        return [=, this](u64 keys) {
            if (keys & key) {
                return operationBlock([&]() {
                    getOffset();
                });
            }
            return false;
        };
    };

public:
    NtpGui() {
        std::string iniFile = iniLocations[0];
        
        // Find the first existing INI file
        for (const char* loc : iniLocations) {
            if (ult::isFileOrDirectory(loc)) {
                iniFile = loc;
                break;
            }
        }

        // Get all key-value pairs from the Servers section
        auto serverMap = ult::getKeyValuePairsFromSection(iniFile, iniSection);

        // Populate server lists from the parsed data
        for (const auto& [key, value] : serverMap) {
            serverAddresses.push_back(value);
            
            std::string keyStr = key;
            std::replace(keyStr.begin(), keyStr.end(), '_', ' ');
            serverNames.push_back(keyStr);
        }

        // Add default server if none were found
        if (serverNames.empty() || serverAddresses.empty()) {
            serverNames.push_back(defaultServerName);
            serverAddresses.push_back(defaultServerAddress);
        }
    }

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("QuickNTP", std::string("by NedEX - v") + APP_VERSION);
        frame->m_showWidget = true;

        auto list = new tsl::elm::List();

        list->addItem(new tsl::elm::CategoryHeader("Pick server "+ult::DIVIDER_SYMBOL+" \uE0E0  Sync "+ult::DIVIDER_SYMBOL+" \uE0E3  Offset"));

        // Create NamedStepTrackBar with V2 style using the server names
        tsl::elm::NamedStepTrackBar* trackbar;
        if (!serverNames.empty()) {
            // Build initializer list from vector
            switch (serverNames.size()) {
                case 1:
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0]}, true, "Server");
                    break;
                case 2:
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0], serverNames[1]}, true, "Server");
                    break;
                case 3:
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0], serverNames[1], serverNames[2]}, true, "Server");
                    break;
                case 4:
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0], serverNames[1], serverNames[2], serverNames[3]}, true, "Server");
                    break;
                case 5:
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0], serverNames[1], serverNames[2], serverNames[3], serverNames[4]}, true, "Server");
                    break;
                default:
                    // For more than 5 servers, just use the first 5
                    trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {serverNames[0], serverNames[1], serverNames[2], serverNames[3], serverNames[4]}, true, "Server");
                    break;
            }
        } else {
            trackbar = new tsl::elm::NamedStepTrackBar("\uE017", {defaultServerName}, true, "Server");
        }
        
        trackbar->setValueChangedListener([this](u8 val) {
            currentServer = val;
        });
        trackbar->setClickListener([this, trackbar](u64 keys) {
            static bool wasTriggered = false;
            
            // Only trigger animation on initial press (keys down), not while held
            if (((keys & HidNpadButton_A) || (keys & HidNpadButton_Y)) && !wasTriggered) {
                trackbar->triggerClickAnimation();
                triggerEnterFeedback();
                wasTriggered = true;
            }
            
            // Reset flag when key is released
            if (!(keys & HidNpadButton_A) && !(keys & HidNpadButton_Y)) {
                wasTriggered = false;
            }
            
            return syncListener(HidNpadButton_A)(keys) || offsetListener(HidNpadButton_Y)(keys);
        });
        list->addItem(trackbar);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {}), 24);

        auto* syncTimeItem = new tsl::elm::ListItem("Sync time");
        syncTimeItem->setClickListener(syncListener(HidNpadButton_A));
        list->addItem(syncTimeItem);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                          renderer->drawString("Syncs the time with the selected server.", false, x + 20, y + 26, 15, renderer->a(tsl::style::color::ColorDescription));
                      }),
                      50);

        auto* getOffsetItem = new tsl::elm::ListItem("Get offset");
        getOffsetItem->setClickListener(offsetListener(HidNpadButton_A));
        list->addItem(getOffsetItem);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                          renderer->drawString("Gets the seconds offset with the selected server.\n\n\uE016  A value of ± 3 seconds is acceptable.", false, x + 20, y + 26, 15, renderer->a(tsl::style::color::ColorDescription));
                      }),
                      70);

        auto* setToInternalItem = new tsl::elm::ListItem("User-set time");
        setToInternalItem->setClickListener([this](u64 keys) {
            if (keys & HidNpadButton_A) {
                return operationBlock([&]() {
                    setNetworkTimeAsUser();
                });
            }
            return false;
        });
        list->addItem(setToInternalItem);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                          renderer->drawString("Sets the network time to the user-set time.", false, x + 20, y + 26, 15, renderer->a(tsl::style::color::ColorDescription));
                      }),
                      50);

        frame->setContent(list);
        return frame;
    }
};

class NtpOverlay : public tsl::Overlay {
public:
    virtual void initServices() override {
        constexpr SocketInitConfig socketInitConfig = {
            // TCP buffers
            .tcp_tx_buf_size     = 16 * 1024,   // 16 KB default
            .tcp_rx_buf_size     = 16 * 1024*2,   // 16 KB default
            .tcp_tx_buf_max_size = 64 * 1024,   // 64 KB default max
            .tcp_rx_buf_max_size = 64 * 1024*2,   // 64 KB default max
            
            // UDP buffers
            .udp_tx_buf_size     = 512,         // 512 B default
            .udp_rx_buf_size     = 512,         // 512 B default
        
            // Socket buffer efficiency
            .sb_efficiency       = 1,           // 0 = default, balanced memory vs CPU
                                                // 1 = prioritize memory efficiency (smaller internal allocations)
            .bsd_service_type    = BsdServiceType_Auto // Auto-select service
        };
        socketInitialize(&socketInitConfig);
        ASSERT_FATAL(nifmInitialize(NifmServiceType_User));
        ASSERT_FATAL(timeInitialize());
        ASSERT_FATAL(smInitialize());
    }

    virtual void exitServices() override {
        socketExit();
        nifmExit();
        timeExit();
        smExit();
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<NtpGui>();
    }
};

int main(int argc, char** argv) {
    return tsl::loop<NtpOverlay>(argc, argv);
}