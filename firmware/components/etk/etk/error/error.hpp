#pragma once

#include <expected>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <esp_err.h>

namespace etk::error {

template<typename T>
using Expected = std::expected<T, esp_err_t>;

class Exception : public std::runtime_error {
public:
    Exception(esp_err_t error)
#if CONFIG_ESP_ERR_TO_NAME_LOOKUP
        : runtime_error(esp_err_to_name(error))
#elif
        : runtime_error("<unavailable>")
#endif
        , m_error(error) {
    }

    esp_err_t error() const {
        return m_error;
    }

private:
    esp_err_t m_error;
};

namespace detail {

void must_failed_print(esp_err_t, const char* file, int line, const char* function, const char* expression);

}

}

#define TRY(expression)                                                     \
    ({                                                                      \
        _Pragma("GCC diagnostic ignored \"-Wshadow\"");                     \
        _Pragma("GCC diagnostic push");                                     \
        auto&& _temporary_fallible = (expression);                          \
        _Pragma("GCC diagnostic pop");                                      \
        if (not _temporary_fallible.has_value()) [[unlikely]]               \
            return std::unexpected(std::move(_temporary_fallible).error()); \
        *std::move(_temporary_fallible);                                    \
    })

#define TRY_RAW(expression)                                \
    do {                                                   \
        _Pragma("GCC diagnostic ignored \"-Wshadow\"");    \
        _Pragma("GCC diagnostic push");                    \
        esp_err_t _temporary_error_code = (expression);    \
        _Pragma("GCC diagnostic pop");                     \
        if (_temporary_error_code != ESP_OK) [[unlikely]]  \
            return std::unexpected(_temporary_error_code); \
    } while (false)

#if CONFIG_COMPILER_CXX_EXCEPTIONS
#    define TRY_OR_THROW(expression)                                        \
        ({                                                                  \
            _Pragma("GCC diagnostic ignored \"-Wshadow\"");                 \
            _Pragma("GCC diagnostic push");                                 \
            auto&& _temporary_fallible = (expression);                      \
            _Pragma("GCC diagnostic pop");                                  \
            if (not _temporary_fallible.has_value()) [[unlikely]]           \
                throw ::etk::error::Exception(_temporary_fallible.error()); \
            *std::move(_temporary_fallible);                                \
        })

#    define TRY_RAW_OR_THROW(expression)                              \
        do {                                                          \
            _Pragma("GCC diagnostic ignored \"-Wshadow\"");           \
            _Pragma("GCC diagnostic push");                           \
            esp_err_t _temporary_error_code = (expression);           \
            _Pragma("GCC diagnostic pop");                            \
            if (_temporary_error_code != ESP_OK) [[unlikely]]         \
                throw ::etk::error::Exception(_temporary_error_code); \
        } while (false)
#endif

#define MUST(expression)                                                                                                                \
    ({                                                                                                                                  \
        _Pragma("GCC diagnostic ignored \"-Wshadow\"");                                                                                 \
        _Pragma("GCC diagnostic push");                                                                                                 \
        auto&& _temporary_fallible = (expression);                                                                                      \
        _Pragma("GCC diagnostic pop");                                                                                                  \
        if (not _temporary_fallible.has_value()) [[unlikely]] {                                                                         \
            ::etk::error::detail::must_failed_print(_temporary_fallible.error(), __FILE__, __LINE__, __PRETTY_FUNCTION__, #expression); \
            std::abort();                                                                                                               \
        }                                                                                                                               \
        *std::move(_temporary_fallible);                                                                                                \
    })
