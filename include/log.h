#pragma once
#include <fmt/format.h>
#include <fmt/std.h>
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <exception>
#include <experimental/source_location>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <string>
#include <string_view>

// 使用环境变量控制特定模块的日志级别:
//
// set global level to debug:
// export SPDLOG_LEVEL=debug
//
// turn off all logging except for logger1:
// export SPDLOG_LEVEL="*=off,logger1=debug"
//
// turn off all logging except for logger1 and logger2:
// export SPDLOG_LEVEL="off,logger1=debug,logger2=info"

static constexpr size_t MAX_FILE_SIZE = 1024 * 1024 * 10;  //  10Mb
static constexpr size_t MAX_FILE_COUNT = 10;
static constexpr std::string_view BASE_FILE_NAME = "logs/running.log";
static constexpr std::string_view MAIN_LOGGER = "main_logger";

using source_location = std::experimental::source_location;
[[nodiscard]] constexpr auto get_log_source_location(const source_location &location) {
    return spdlog::source_loc{
        location.file_name(), static_cast<std::int32_t>(location.line()), location.function_name()};
}
struct format_with_location {
    std::string_view value;
    spdlog::source_loc loc;
    template <typename String>
    format_with_location(const String &s, const source_location &location = source_location::current())
        : value{s},
          loc{get_log_source_location(location)} {}
};

using LOG_LEVEL = spdlog::level::level_enum;
template <typename... Args>
void spd_log_raw(spdlog::logger *logger, LOG_LEVEL log_level, format_with_location fmt_, Args &&...args) {
    logger->log(
        fmt_.loc, log_level, fmt::runtime(fmt_.value), std::forward<Args>(args)...
    );
}
template <typename... Args>
void spd_log(LOG_LEVEL log_level, format_with_location fmt_, Args &&...args) {
    spd_log_raw(spdlog::default_logger_raw(), log_level, fmt_, std::forward<Args>(args)...);
}
// #ifndef NDEBUG
#ifdef DEBUG
#include <cpptrace/cpptrace.hpp>
#define LOG(level, fmt_with_loc, ...) spd_log(level, fmt_with_loc, ##__VA_ARGS__)
#define PRINT_STACK_TRACE() { cpptrace::generate_trace().print(); }
#else
#define LOG(level, fmt_with_loc, ...)
#define PRINT_STACK_TRACE()
#if defined(assert)
#undef assert
#define assert(expr) { if(!(expr)) { std::abort(); } }
#endif  // !defined(assert)
#endif

#define LOG_TRACE(str, ...) LOG(LOG_LEVEL::trace, str, ##__VA_ARGS__)
#define LOG_DEBUG(str, ...) LOG(LOG_LEVEL::debug, str, ##__VA_ARGS__)
#define LOG_INFO(str, ...) LOG(LOG_LEVEL::info, str, ##__VA_ARGS__)
#define LOG_WARN(str, ...) LOG(LOG_LEVEL::warn, str, ##__VA_ARGS__)
#define LOG_ERROR(str, ...) LOG(LOG_LEVEL::err, str, ##__VA_ARGS__)
#define LOG_FATAL(str, ...) { LOG(LOG_LEVEL::critical, str, ##__VA_ARGS__); PRINT_STACK_TRACE(); assert(false); }
#define LOG_PRINT(str, ...) { spd_log(static_cast<LOG_LEVEL>(SPDLOG_LEVEL_INFO), str, ##__VA_ARGS__); }
#define LOG_DISABLED(str, ...)
#define LOG_COND(level, expr, str, ...) { if(expr) { LOG(level, str, ##__VA_ARGS__); } }
#define LOG_ASSERT(expr, str, ...) { if(!(expr)) { LOG_FATAL(str, ##__VA_ARGS__); } }
#define ASSERT(expr) { LOG_ASSERT(expr, ""); }
#define ASSERT_FATAL(expr) { if constexpr (SPDLOG_ACTIVE_LEVEL <= LOG_LEVEL::err) { if(!(expr)) { LOG_FATAL(""); } } }


inline void terminate_handler() {
    std::exception_ptr exptr = std::current_exception();
    if (exptr != nullptr) {
        try {
            std::rethrow_exception(exptr);
        } catch (const std::exception& e) {
            LOG_ERROR("Uncaught exception: {}", e.what());
        } catch (...) {
            LOG_ERROR("Uncaught exception of unknown type");
        }
    }
    PRINT_STACK_TRACE();
    std::abort();
}

/**
 * @brief 初始化全局logger
 */
inline void init_logger() {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        // std::string log_pattern = "[%Y-%m-%d %T.%e][%^%l%$][%s:%#][%!] %v";
        // [17:29:31.540][info][main_logger][log.h:94][init_logger] initialized
        // %n: 模块名, 构造logger时传入
        std::string log_pattern = "[%T.%e][%^%l%$][%n][%s:%#][%!] %v";

        // 每一个logger和sink都是可以单独设置日志格式和日志级别的
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(log_pattern);
        console_sink->set_level(spdlog::level::info);

        // rotating_file_sink_mt当文件达到指定大小时会自动创建新文件
        // auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        //     std::string(BASE_FILE_NAME), MAX_FILE_SIZE, MAX_FILE_COUNT
        // );
        //
        // basic_file_sink_mt; true: 每次启动程序时都会清空文件
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::string(BASE_FILE_NAME), true);
        file_sink->set_pattern(log_pattern);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        const auto main_logger = std::make_shared<spdlog::logger>(std::string(MAIN_LOGGER), std::begin(sinks), std::end(sinks));
        main_logger->flush_on(spdlog::level::debug);
        spdlog::flush_every(std::chrono::seconds(1));
        main_logger->set_level(spdlog::level::trace);
        /* set default */
        set_default_logger(main_logger);

        spdlog::cfg::load_env_levels();

        std::set_terminate(terminate_handler);
    });
}
#ifndef NDEBUG
#define INIT_LOGGER() \
    { init_logger(); }
#else
#define INIT_LOGGER()
#endif

// #ifndef NDEBUG
#ifdef DEBUG
class ModuleLogger {
  private:
    std::shared_ptr<spdlog::logger> logger_;
    std::string module_name_;

    template <typename... Args>
    void module_log(spdlog::logger *logger, LOG_LEVEL log_level, format_with_location fmt, Args &&...args) const {
        spd_log_raw(logger, log_level, fmt, std::forward<Args>(args)...);
    }

  protected:
    std::shared_ptr<spdlog::logger> get_logger() { return logger_; }

  public:
    /**
     * @param level 如果没有level参数就使用现有logger默认的; 如果有则进行设置; 环境变量优先级最高
     * @todo 重复构造的情况下set_level会失效
     */
    explicit ModuleLogger(const std::string &module_name, std::optional<LOG_LEVEL> level = std::nullopt): module_name_(module_name) {
        INIT_LOGGER();
        logger_ = spdlog::get(module_name);
        if (logger_) {
            // 如果提供了level参数，则设置级别
            if (level.has_value()) {
                set_level(*level);
                if (level != logger_->level()) {
                    LOG_TRACE("同一模块最后一次设置的level会覆盖之前的; 当前: {}, 设置: {}", spdlog::level::to_string_view(logger_->level()), spdlog::level::to_string_view(*level));
                }
            }
        } else {
            // create new logger with same sinks and configuration.
            logger_ = spdlog::get(std::string(MAIN_LOGGER))->clone(module_name);
            // 如果提供了level参数，才设置级别
            if (level.has_value()) {
                set_level(*level);
            }
            spdlog::register_logger(logger_);
            // 要先注册再加载环境变量
            LOG_INFO("logger [{}] registered with level [{}]", logger_->name(), spdlog::level::to_string_view(logger_->level()));
        }
        // 环境变量优先级最高
        spdlog::cfg::load_env_levels();
    }
    template <typename... Args>
    void disabled(format_with_location fmt, Args &&...args) const {}
    template <typename... Args>
    void trace(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::trace, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::debug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void info(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::warn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::err, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void fatal(format_with_location fmt, Args &&...args) const {
        module_log(logger_.get(), LOG_LEVEL::critical, fmt, std::forward<Args>(args)...);
        PRINT_STACK_TRACE();
    }
    template <typename... Args>
    void cond(LOG_LEVEL level, bool log_if_true, format_with_location fmt, Args &&...args) const {
        if (log_if_true) {
            module_log(logger_.get(), level, fmt, std::forward<Args>(args)...);
        }
    }
    // 为了不与assert宏冲突
    template <typename... Args>
    void expect(bool assert_if_false, format_with_location fmt, Args &&...args) const {
        cond(LOG_LEVEL::critical, !assert_if_false, fmt, std::forward<Args>(args)...);
        assert(assert_if_false);
    }

    void set_level(LOG_LEVEL level) { logger_->set_level(level); }

    void load_env_levels() { spdlog::cfg::load_env_levels(); }

    LOG_LEVEL level() const { return logger_->level(); }

    std::string name() const { return logger_->name(); }
};
#else
class ModuleLogger {
    std::shared_ptr<spdlog::logger> logger_;
    std::string module_name_;

    template <typename... Args>
    void module_log(spdlog::logger *logger, LOG_LEVEL log_level, format_with_location fmt, Args &&...args) const {}

  protected:
    // release下不允许获取内部的logger
    std::shared_ptr<spdlog::logger> get_logger() { return nullptr; }

  public:
    explicit ModuleLogger(const std::string &module_name, std::optional<LOG_LEVEL> level = std::nullopt) {}
    template <typename... Args>
    void disabled(format_with_location, Args &&...) const {}
    template <typename... Args>
    void trace(format_with_location, Args &&...) const {}
    template <typename... Args>
    void debug(format_with_location, Args &&...) const {}
    template <typename... Args>
    void info(format_with_location, Args &&...) const {}
    template <typename... Args>
    void warn(format_with_location, Args &&...) const {}
    template <typename... Args>
    void error(format_with_location, Args &&...) const {}
    template <typename... Args>
    void fatal(format_with_location, Args &&...) const {}
    template <typename... Args>
    void cond(LOG_LEVEL level, bool log_if_true, format_with_location fmt, Args &&...args) const {}
    template <typename... Args>
    void expect(bool assert_if_false, format_with_location fmt, Args &&...args) const { assert(assert_if_false); }
    void set_level(LOG_LEVEL) {}
    void load_env_levels() {}
    LOG_LEVEL level() const { return LOG_LEVEL::off; }
    std::string name() const { return ""; }
};
#endif

// 由于使用函数进行日志调用时, Release下函数参数依然会被求值, 十分影响性能, 因此使用宏对ModuleLogger进行封装

/*
#define MODULE_LOGGER(logger, level, fmt, ...) \
    logger.level(fmt, ##__VA_ARGS__);
*/
#ifdef DEBUG

#define REGISTER_MODULE_LOGGER(module_name, level) \
    ModuleLogger logger = ModuleLogger(module_name, level)

#define logger_disabled(fmt, ...) logger.disabled(fmt, ##__VA_ARGS__)
#define logger_trace(fmt, ...) logger.trace(fmt, ##__VA_ARGS__)
#define logger_info(fmt, ...) logger.info(fmt, ##__VA_ARGS__)
#define logger_debug(fmt, ...) logger.debug(fmt, ##__VA_ARGS__)
#define logger_warn(fmt, ...) logger.warn(fmt, ##__VA_ARGS__)
#define logger_error(fmt, ...) logger.error(fmt, ##__VA_ARGS__)
#define logger_fatal(fmt, ...) logger.fatal(fmt, ##__VA_ARGS__)
#define logger_cond(level, expr, fmt, ...) { logger.cond(level, expr, fmt, ##__VA_ARGS__); }
#define logger_expect(expr, fmt, ...) { logger.expect(expr, fmt, ##__VA_ARGS__); }

#else

#define REGISTER_MODULE_LOGGER(module_name, level)

#define logger_disabled(fmt, ...)
#define logger_trace(fmt, ...)
#define logger_info(fmt, ...)
#define logger_debug(fmt, ...)
#define logger_warn(fmt, ...)
#define logger_error(fmt, ...)
#define logger_fatal(fmt, ...)
#define logger_cond(level, expr, fmt, ...)
#define logger_expect(expr, fmt, ...) { assert(expr); }

#endif

