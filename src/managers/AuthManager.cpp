#include "AuthManager.hpp"
#include "ThumbnailManager.hpp"

#include <Geode/Result.hpp>
#include <matjson.hpp>
#include <string>
#include <argon/argon.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

bool AuthManager::isLoggedIn() {
    return Mod::get()->hasSavedValue("token");
}

ThumbnailRole getRoleByName(std::string_view role) {
    auto it = THUMBNAIL_ROLE_SERVER_NAMES.find(role);
    if (it != THUMBNAIL_ROLE_SERVER_NAMES.end()) {
        return it->second;
    }
    return ThumbnailRole::NONE;
}

std::optional<ThumbnailRoleInfo> getRoleInfoByName(std::string_view role) {
    auto r = getRoleByName(role);
    if (r == ThumbnailRole::NONE) {
        return std::nullopt;
    }
    return THUMBNAIL_ROLES[static_cast<size_t>(r) - 1];
}

AuthManager& AuthManager::get() {
    static AuthManager instance;
    return instance;
}

static std::string getErrorMessage(web::WebResponse const& res) {
    if (auto jsonRes = res.json()) {
        auto json = std::move(jsonRes).unwrap();

        std::string message;
        if (json["reason"].isString() && res.code() != 403) {
            message = json["reason"].asString().unwrap();
        } else if (json["message"].isString()) {
            message = json["message"].asString().unwrap();
        }

        std::string details;
        if (json["details"].isString()) {
            details = json["details"].asString().unwrap();
        }

        if (!message.empty()) {
            if (!details.empty()) {
                return fmt::format("{}: <co>{}</c>", message, details);
            }
            return message;
        }
    }

    StringBuffer<> buf("HTTP ");
    buf.append("<cy>{}</c>", res.code());

    auto str = res.string().unwrapOrDefault();
    if (!str.empty()) {
        buf.append(": <cj>{}</c>", str);
    }

    if (!res.errorMessage().empty()) {
        buf.append(" (<cr>{}</c>)", res.errorMessage());
    }

    return buf.str();
}

AuthManager::UploadFuture AuthManager::uploadThumbnail(asp::fs::Path filename, int levelID, std::string note, Function<void(ZStringView)> onProgress) {
    if (onProgress) onProgress("Logging in...");

    if (!this->isLoggedIn()) {
        GEODE_CO_UNWRAP(co_await this->login());
    }

    if (m_serverMetadata.has_value()) {
        auto data = co_await async::waitForMainThread<argon::AccountData>([] {
            return argon::getGameAccountData();
        });

        if (data->serverUrl != m_serverMetadata->gdBaseUrl) {
            co_return Err("Incompatible Geometry Dash server URL: <cr>{}</c> (expected <cy>{}</c>)", data->serverUrl, m_serverMetadata->gdBaseUrl);
        }
    }

    if (onProgress) onProgress("Uploading...");

    auto token = this->getToken();
    auto res = co_await web::WebRequest()
        .header("Authorization", fmt::format("Bearer {}", token))
        .userAgent(USER_AGENT)
        .param("level_id", levelID)
        .param("note", note)
        .file("file", filename)
        .post(fmt::format("{}/thumbnail/upload", Settings::thumbnailAPIBaseURL()));

    if (!res.ok()) {
        if (res.code() == 401) {
            Mod::get()->getSaveContainer().erase("token");
            co_return Err("Unauthorized. Please try again.");
        }
        co_return Err(getErrorMessage(res));
    }

    auto code = res.code();

    if (code == 201 || code == 200) {
        co_return Ok("The thumbnail has been <cg>applied</c>.");
    }

    if (code == 202) {
        co_return Ok("The thumbnail has been <co>submitted</c>, and is now <cj>in the queue</c> for approval.");
    }

    co_return Err(getErrorMessage(res));
}
