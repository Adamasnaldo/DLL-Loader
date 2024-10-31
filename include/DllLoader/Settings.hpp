#pragma once

#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.h>

#include <variant>
#include <locale>
#include <codecvt>
#include <string>
#include <filesystem>


namespace DllLoader {

    namespace fs = std::filesystem;

    static std::optional<const toml::node* const> node_at_path(const toml::table& root, const std::initializer_list<std::string>& path) {
        if (path.size( ) == 0) {
            return (toml::node*) &root;
        }

        std::vector<std::string> keys = path;
        const toml::table* container = root.as_table( );

        while (!keys.empty( )) {
            const std::string key = keys[0];
            keys.erase(keys.begin( ));

            if (!container->contains(key)) {
                return {};
            }

            const toml::node* value = container->get(key);
            if (keys.empty( )) {
                return value;
            }

            container = value->as_table( );
        }

        return {};
    }

    class ConfigReader;
    template <typename T>
    concept ConfigObject = requires(T obj, const ConfigReader& t) {
        {
            obj.from_toml(t)
        } -> std::same_as<bool>;
    };

    class ConfigReader {
    private:
        template <typename T>
        const std::optional<const T> node_to_val(const toml::node* const node) const {
            return node->value<T>( );
        }

        template <bool ignoreInvalid = true>
        const std::optional<const fs::path> node_to_val(const toml::node* const node) const {
            fs::path fs_path = fs::path(node->value<std::wstring>( ).value( ));

            // if we don't have an absolute path, relativize it to the path we loaded configuration from
            if (!fs_path.is_absolute( )) {
                fs_path = relativePath / fs_path;
            }

            if (ignoreInvalid && !fs::exists(fs_path)) {
                return {};
            }

            return fs_path.lexically_normal( );
        }

    public:
        ConfigReader(const toml::table* const _root, const std::filesystem::path& _relPath)
            : root(_root)
            , relativePath(_relPath) {
        }


#define READ_CONFIG_OPTION( T ) \
        const auto node = node_at_path(*this->root, path); \
        if (!node) { \
            return std::nullopt; \
        } \
        return node_to_val<T>(*node);

        template <typename T>
        inline const std::optional<T> read_config_option(const std::initializer_list<std::string>& path) const {
            READ_CONFIG_OPTION(T);
        }
        template <bool ignoreInvalid = true>
        inline const std::optional<fs::path> read_config_option(const std::initializer_list<std::string>& path) const {
            READ_CONFIG_OPTION(ignoreInvalid);
        }


#define READ_CONFIG_OPTIONS( OPTIONS, T ) \
        const auto node = node_at_path(*this->root, path); \
        if (!node || !(*node)->is_array( )) { \
            return OPTIONS; \
        } \
        for (const toml::node& el : *(*node)->as_array( )) { \
            const auto el_value = node_to_val<T>(&el); \
            if (el_value) { \
                OPTIONS.push_back(*el_value); \
            } \
        } \
        return OPTIONS;

        template <typename T>
        inline const std::vector<T> read_config_options(const std::initializer_list<std::string>& path) const {
            std::vector<T> options;
            READ_CONFIG_OPTIONS(options, T);
        }
        template <bool ignoreInvalid = true>
        inline const std::vector<fs::path> read_config_options(const std::initializer_list<std::string>& path) const {
            std::vector<fs::path> options;
            READ_CONFIG_OPTIONS(options, ignoreInvalid);
        }


        template <ConfigObject T>
        const std::vector<T> read_config_objects(const std::initializer_list<std::string>& path) const {
            std::vector<T> results;

            const auto node = node_at_path(*this->root, path);
            if (!node || !(*node)->is_array_of_tables( )) {
                return results;
            }

            const toml::array array = (*node)->as_array( );
            for (const auto& obj_node : *array) {
                const T obj = T( );
                const ConfigReader reader = ConfigReader(obj_node.as_table( ), this->relativePath);

                if (obj.from_toml(reader)) {
                    results.push_back(obj);
                }
            }

            return results;
        }

        template <ConfigObject T>
        const T read_config_object(const std::initializer_list<std::string>& path) const {
            T obj = T( );
            const auto node = node_at_path(*this->root, path);

            if (node && (*node)->is_table( )) {
                const toml::table* table = (*node)->as_table( );
                const ConfigReader reader(table, this->relativePath);

                obj.from_toml(reader);
            }

            return obj;
        }

    private:
        const toml::table* const root;
        const std::filesystem::path relativePath;
    };

    class Settings {
    public:
        Settings(const fs::path _loaderDir, const toml::table& _config) : loaderDir(_loaderDir), config(_config) {
        }

        inline ConfigReader getConfigReader( ) const {
            return ConfigReader(&this->config, this->loaderDir);
        }

    private:
        const toml::table config;

        const fs::path loaderDir;
    };
}