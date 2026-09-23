#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "../value.hpp"
#include "../font/ttf.hpp"
#include "../string_type.hpp"

namespace modoc {
    namespace font_obj {
        inline size_t loaded_fonts = 0;

        struct resource_reference {
            //modoc::string_type path; TODO: use this instead of string
            std::string path;
            std::unique_ptr<ttf_resource> resource = nullptr;

            resource_reference(std::string_view path) : path(path) {}

            ttf_resource* get_resource() {
                if (resource == nullptr) {
                    resource = std::make_unique<ttf_resource>();
                    *resource = load_ttf(path.data());
                    std::cout << "Loaded fonts: " << ++loaded_fonts << '\n';
                }
                return resource.get();
            }
        };

        inline std::vector<resource_reference> resources;

        static size_t load(std::string_view path) {
            std::cout << "Loading font: " << path << " Loaded: " << resources.size() << '\n';
            resources.emplace_back(path);
            return resources.size() - 1;
        }

        static void init() {
            value::object_t obj;
            
            obj["load"] = value::from_function([](const value::object_t& obj) -> value {
                auto itr = obj.find("src");

                if (itr != obj.end() && itr->second.type() == value::STRING) {
                    std::string_view src = itr->second.string();
                    return load(src);
                }

                return value::null();
            });

            obj["JetBrainsMono"] = load("font/JetBrainsMono/static/JetBrainsMono-Regular.ttf");
            obj["FunnelDisplay"] = load("font/Funnel_Display/static/FunnelDisplay-Regular.ttf");
            obj["GoogleSansCode"] = load("font/Google_Sans_Code/static/GoogleSansCode-Regular.ttf");
            obj["DMSerifText"] = load("font/DM_Serif_Text/DMSerifText-Regular.ttf");
            obj["Gelasio"] = value::from_object({
                {"Regular", load("font/Gelasio/static/Gelasio-Regular.ttf")},
                {"Bold", load("font/Gelasio/static/Gelasio-Bold.ttf")},
                {"SemiBold", load("font/Gelasio/static/Gelasio-SemiBold.ttf")}
            });

            std::cout << "Resources: " << resources.size() << '\n';

            register_constant("font", value::from_object(std::move(obj)));
        }

        static ttf_resource* get_resource(size_t id) {
            if (id >= resources.size()) return nullptr;
            return resources[id].get_resource();
        }
    };
};
