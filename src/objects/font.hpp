#pragma once

#include "../value.hpp"
#include "../font/ttf.hpp"
#include <string_view>
#include <vector>

namespace modoc {
    namespace font_obj {
        inline std::vector<ttf_resource> resources;

        static size_t load(std::string_view path) {
            std::cout << "Loading font. Loaded: " << resources.size() << '\n';
            resources.push_back(load_ttf(path.data()));
            std::cout << "Loaded fonts: " << resources.size() << '\n';
            return resources.size() - 1;
        }

        static void init() {
            value::object_t obj;
            
            obj["JetBrainsMono"] = load("font/JetBrainsMono/static/JetBrainsMono-Regular.ttf");
            obj["FunnelDisplay"] = load("font/Funnel_Display/static/FunnelDisplay-Regular.ttf");
            obj["GoogleSansCode"] = load("font/Google_Sans_Code/static/GoogleSansCode-Regular.ttf");
            obj["DMSerifText"] = load("font/DM_Serif_Text/DMSerifText-Regular.ttf");
            obj["Gelasio"] = load("font/Gelasio/static/Gelasio-Regular.ttf");

            std::cout << "Resources: " << resources.size() << '\n';

            register_constant("font", value::from_object(std::move(obj)));
        } 
    };
};
