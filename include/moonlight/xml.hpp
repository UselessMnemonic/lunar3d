#pragma once

#include <string>
#include <vector>

namespace lunar3d {
namespace moonlight {

enum class GameStreamResult {
    Ok,
    ParseError,
    StatusError,
    MissingField,
    InvalidField,
};

struct GameStreamApp {
    int id = 0;
    std::string title;
};

struct GameStreamDisplayMode {
    int width = 0;
    int height = 0;
    int refreshRate = 0;
};

struct GameStreamServerInfo {
    bool paired = false;
    int currentGame = 0;
    int httpsPort = 47984;
    int serverCodecModeSupport = 0;
    std::string appversion;
    std::string gfeVersion;
    std::string state;
    std::string gputype;
    std::string gsVersion;
    std::vector<GameStreamDisplayMode> displayModes;
};

GameStreamResult parseStatus(const std::string& xml);
GameStreamResult parseServerInfo(const std::string& xml, GameStreamServerInfo& info);
GameStreamResult parseAppList(const std::string& xml, std::vector<GameStreamApp>& apps);
GameStreamResult findText(const std::string& xml, const char* elementName, std::string& text);

const char* toString(GameStreamResult result);

} // namespace moonlight
} // namespace lunar3d
