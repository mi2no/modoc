#pragma once

#include <cmath>
#include <numbers>

#include "../node.hpp"

struct math_f : node_factory {
    
    void init() override {
        value obj = value::from_object({});
        value::object_t& map = obj.object();

        {
            map["pi"] = std::numbers::pi;

            map["sin"] = value::from_function([](const value::object_t& map) -> value {
                auto it = map.find("x");
                if (it != map.end()) {
                    if (it->second.type() == value::NUMBER) return std::sin(it->second.number());
                    else if (it->second.type() == value::OBJECT && it->second.object().contains("value") && it->second.object().at("value").type() == value::NUMBER) {
                        const value::object_t& obj = it->second.object();
                        double number = obj.at("value").number();

                        if (obj.contains("unit") && obj.at("unit").type() == value::STRING && obj.at("unit").string() == "deg") number = number * std::numbers::pi / 180;
                        
                        return std::sin(number);
                    }
                }

                return value::null();
            });

            map["deg"] = value::from_function([](const value::object_t& map) -> value {
                auto it = map.find("x");
                if (it != map.end()) return value::from_object({{"value", it->second}, {"unit", value("deg")}});

                return value::null();
            });

        }

        register_constant("math", std::move(obj));
    }

    virtual node* instance(uint8_t, const options_t&) override {
        return nullptr;
    }

    void set_node_type_id(uint32_t id) const override {}
};
