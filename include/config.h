#pragma once

#include <string>
#include "log.h"

struct CONFIG {
    static inline std::string_view ENDPOINT = "obs.cn-east-3.myhuaweicloud.com";

    static inline std::string_view BUCKET_LOCATION = "cn-east-3";

    static inline std::string_view BUCKET_NAME = "hw-obs-bench";

    static inline std::string_view ACCESS_KEY_ID = "";

    static inline std::string_view SECRET_ACCESS_KEY = "";

    template <typename T>
    inline static void init_config(T &config, std::string_view config_name) {
        std::string_view config_name_sv = config_name.substr(config_name.find("::") + 2);
        std::string env_name = "CONFIG_" + std::string(config_name_sv.data());
        char *env = getenv(env_name.c_str());
        if (env) {
            if constexpr (std::is_same<T, std::string_view>::value) {
                config = std::string_view(env);
            } else {
                config = static_cast<T>(std::stoi(env));
            }
        }
        LOG_PRINT("{} = {}", env_name, config);
    }
};

#define INIT_CONFIG(name)            \
    {                                               \
        constexpr std::string_view config_name = #name;               \
        CONFIG::init_config(name, config_name.data()); \
    }

inline void init_all_config() {
    INIT_CONFIG(CONFIG::ENDPOINT);
    INIT_CONFIG(CONFIG::BUCKET_LOCATION);
    INIT_CONFIG(CONFIG::BUCKET_NAME);
    INIT_CONFIG(CONFIG::ACCESS_KEY_ID);
    INIT_CONFIG(CONFIG::SECRET_ACCESS_KEY);
}