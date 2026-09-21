#pragma once

#include "../value.hpp"
#include "../font/ttf.hpp"
#include <string_view>
#include <vector>

namespace modoc {
    namespace font_obj {
        static std::vector<ttf_resource> resources;

        static size_t load(std::string_view path) {
            resources.push_back(load_ttf(path.data()));
            return resources.size() - 1;
        }

        static void init() {
            value::object_t obj;
            
            obj["JetBrainsMono"] = load("../font/JetBrainsMono/static/JetBrainsMono-Regular.ttf"); 

            register_constant("font", value::from_object(std::move(obj)));
        } 
    };
};
