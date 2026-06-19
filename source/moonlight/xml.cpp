#include "moonlight/xml.hpp"

#include <tinyxml2.h>

#include <stdlib.h>
#include <string.h>

namespace lunar3d {
namespace moonlight {
namespace {

tinyxml2::XMLElement* findElement(tinyxml2::XMLElement* element, const char* name) {
    if (!element) {
        return nullptr;
    }

    if (strcmp(element->Name(), name) == 0) {
        return element;
    }

    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child;
         child = child->NextSiblingElement()) {
        tinyxml2::XMLElement* found = findElement(child, name);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

tinyxml2::XMLElement* firstElement(tinyxml2::XMLDocument& document, const char* name) {
    return findElement(document.RootElement(), name);
}

GameStreamResult readStatus(tinyxml2::XMLDocument& document) {
    tinyxml2::XMLElement* root = document.RootElement();
    if (!root) {
        return GameStreamResult::ParseError;
    }

    int statusCode = 0;
    tinyxml2::XMLError query = root->QueryIntAttribute("status_code", &statusCode);
    if (query != tinyxml2::XML_SUCCESS) {
        return GameStreamResult::MissingField;
    }

    return statusCode == 200 ? GameStreamResult::Ok : GameStreamResult::StatusError;
}

GameStreamResult loadDocument(const std::string& xml, tinyxml2::XMLDocument& document) {
    tinyxml2::XMLError result = document.Parse(xml.c_str(), xml.size());
    if (result != tinyxml2::XML_SUCCESS) {
        return GameStreamResult::ParseError;
    }

    return readStatus(document);
}

const char* textOrEmpty(tinyxml2::XMLElement* element) {
    const char* text = element ? element->GetText() : nullptr;
    return text ? text : "";
}

bool readInt(tinyxml2::XMLElement* parent, const char* name, int& value) {
    tinyxml2::XMLElement* element = parent ? parent->FirstChildElement(name) : nullptr;
    if (!element || !element->GetText()) {
        return false;
    }

    value = atoi(element->GetText());
    return true;
}

bool readText(tinyxml2::XMLElement* parent, const char* name, std::string& value) {
    tinyxml2::XMLElement* element = parent ? parent->FirstChildElement(name) : nullptr;
    if (!element || !element->GetText()) {
        return false;
    }

    value = element->GetText();
    return true;
}

} // namespace

GameStreamResult parseStatus(const std::string& xml) {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLError result = document.Parse(xml.c_str(), xml.size());
    if (result != tinyxml2::XML_SUCCESS) {
        return GameStreamResult::ParseError;
    }

    return readStatus(document);
}

GameStreamResult parseServerInfo(const std::string& xml, GameStreamServerInfo& info) {
    info = GameStreamServerInfo();

    tinyxml2::XMLDocument document;
    GameStreamResult result = loadDocument(xml, document);
    if (result != GameStreamResult::Ok) {
        return result;
    }

    tinyxml2::XMLElement* root = document.RootElement();
    if (!root) {
        return GameStreamResult::ParseError;
    }

    int pairStatus = 0;
    if (!readInt(root, "PairStatus", pairStatus) ||
        !readInt(root, "currentgame", info.currentGame) ||
        !readText(root, "appversion", info.appversion) || !readText(root, "state", info.state)) {
        info = GameStreamServerInfo();
        return GameStreamResult::MissingField;
    }

    info.paired = pairStatus == 1;
    readInt(root, "HttpsPort", info.httpsPort);
    readInt(root, "ServerCodecModeSupport", info.serverCodecModeSupport);
    readText(root, "GfeVersion", info.gfeVersion);
    readText(root, "gputype", info.gputype);
    readText(root, "GsVersion", info.gsVersion);

    for (tinyxml2::XMLElement* mode = firstElement(document, "DisplayMode"); mode;
         mode = mode->NextSiblingElement("DisplayMode")) {
        GameStreamDisplayMode displayMode;
        if (readInt(mode, "Width", displayMode.width) &&
            readInt(mode, "Height", displayMode.height) &&
            readInt(mode, "RefreshRate", displayMode.refreshRate)) {
            info.displayModes.push_back(displayMode);
        }
    }

    return GameStreamResult::Ok;
}

GameStreamResult parseAppList(const std::string& xml, std::vector<GameStreamApp>& apps) {
    apps.clear();

    tinyxml2::XMLDocument document;
    GameStreamResult result = loadDocument(xml, document);
    if (result != GameStreamResult::Ok) {
        return result;
    }

    tinyxml2::XMLElement* root = document.RootElement();
    if (!root) {
        return GameStreamResult::ParseError;
    }

    for (tinyxml2::XMLElement* app = root->FirstChildElement("App"); app;
         app = app->NextSiblingElement("App")) {
        GameStreamApp item;
        if (!readInt(app, "ID", item.id) || !readText(app, "AppTitle", item.title)) {
            apps.clear();
            return GameStreamResult::InvalidField;
        }

        apps.push_back(item);
    }

    return GameStreamResult::Ok;
}

GameStreamResult findText(const std::string& xml, const char* elementName, std::string& text) {
    text.clear();

    tinyxml2::XMLDocument document;
    GameStreamResult result = loadDocument(xml, document);
    if (result != GameStreamResult::Ok) {
        return result;
    }

    tinyxml2::XMLElement* element = firstElement(document, elementName);
    if (!element) {
        return GameStreamResult::MissingField;
    }

    text = textOrEmpty(element);
    return GameStreamResult::Ok;
}

const char* toString(GameStreamResult result) {
    switch (result) {
    case GameStreamResult::Ok:
        return "ok";
    case GameStreamResult::ParseError:
        return "parse-error";
    case GameStreamResult::StatusError:
        return "status-error";
    case GameStreamResult::MissingField:
        return "missing-field";
    case GameStreamResult::InvalidField:
        return "invalid-field";
    }

    return "unknown";
}

} // namespace moonlight
} // namespace lunar3d
