#include "condition_parser.h"
#include "condition_value.h"
#include "condition_eq.hpp"
#include "condition_gt.hpp"
#include "condition_lt.hpp"
#include "condition_not.hpp"
#include "condition_and.hpp"
#include "condition_or.hpp"
#include "condition_neq.hpp"
#include "condition_gte.hpp"
#include "condition_lte.hpp"
#include "src/parameters/parameter.hpp"

std::unique_ptr<condition_t> parse_condition_node(const std::string &effect_name, json_t *node);
std::unique_ptr<condition_t> parse_condition_array(const std::string &effect_name, json_t *arr);

std::optional<condition_member> parse_condition_member(const std::string &effect_name, json_t *condition_member) {
    auto type = json_typeof(condition_member);
    switch (type) {
        case JSON_STRING: {
            auto param_name = json_string_value(condition_member);
            std::string full_param_name = get_full_param_name_static(effect_name, param_name);
            return full_param_name;
        }
        case JSON_REAL:
        case JSON_INTEGER: {
            return json_number_value(condition_member);
        }
        case JSON_FALSE: {
            return false;
        }
        case JSON_TRUE: {
            return true;
        }

        default: {
            log_error("Invalid condition member. Condition members must be a string referencing another param, a number, or a boolean.");
            return std::nullopt;
        }
    }
}

std::unique_ptr<condition_t> make_leaf_condition(const std::string &effect_name, json_t *lhs, json_t *op, json_t *rhs) {
    std::string op_val = json_string_value(op);
    auto lhs_member = parse_condition_member(effect_name, lhs);
    auto rhs_member = parse_condition_member(effect_name, rhs);
    if (!lhs_member.has_value() || !rhs_member.has_value()) {
        return nullptr;
    }

    if (op_val == "==" || op_val == "eq") {
        return std::make_unique<condition_eq>(*lhs_member, *rhs_member);
    }

    if (op_val == "!=" || op_val == "neq" || op_val == "ne") {
        return std::make_unique<condition_neq>(*lhs_member, *rhs_member);
    }

    if (op_val == ">" || op_val == "gt") {
        return std::make_unique<condition_gt>(*lhs_member, *rhs_member);
    }

    if (op_val == ">=" || op_val == "gte") {
        return std::make_unique<condition_gte>(*lhs_member, *rhs_member);
    }

    if (op_val == "<" || op_val == "lt") {
        return std::make_unique<condition_lt>(*lhs_member, *rhs_member);
    }

    if (op_val == "<=" || op_val == "lte") {
        return std::make_unique<condition_lte>(*lhs_member, *rhs_member);
    }

    log_error("Unknown conditional operator %s; Condition is ignored", op_val.c_str());
    return nullptr;
}

template<typename T> std::unique_ptr<condition_t> parse_condition_group(const std::string &effect_name, json_t *conditions) {
    if (json_typeof(conditions) != JSON_ARRAY) {
        log_error("parse_condition_node: 'and'/'or' node should be an array; json: %s", json_dumps(conditions, 0));
        return nullptr;
    }
    const size_t count = json_array_size(conditions);

    auto result = std::make_unique<T>();
    for (size_t i = 0; i < count; i++) {
        auto *child = json_array_get(conditions, i);
        result->add(parse_condition(effect_name, child));
    }

    return result;
}

std::unique_ptr<condition_t> parse_condition_node(const std::string &effect_name, json_t *node)
{
    if (!node) {
        return nullptr;
    }

    // AND
    {
        auto *and_field = json_object_get(node, "and");
        if (and_field != nullptr) {
            return parse_condition_group<condition_and>(effect_name, and_field);
        }
    }

    // OR
    {
        auto *or_field = json_object_get(node, "or");
        if (or_field != nullptr) {
            return parse_condition_group<condition_or>(effect_name, or_field);
        }
    }

    // NOT
    {
        auto *not_field = json_object_get(node, "not");
        if (not_field != nullptr) {
            auto result = std::make_unique<condition_not>(parse_condition(effect_name, not_field));
            return result;
        }
    }
    return nullptr;
}

std::unique_ptr<condition_t> parse_condition_array(const std::string &effect_name, json_t *arr)
{
    const size_t count = json_array_size(arr);
    if (count != 3) {
        log_error("Invalid condition. Conditions must have 3 values");
        return nullptr;
    }

    auto *lhs_item = json_array_get(arr, 0);
    auto *op_item  = json_array_get(arr, 1);
    auto *rhs_item = json_array_get(arr, 2);

    if (json_typeof(op_item) != JSON_STRING) {
        log_error("Invalid condition. Operator must be a string");
        return nullptr;
    }

    return make_leaf_condition(effect_name, lhs_item, op_item, rhs_item);
}

std::unique_ptr<condition_t> parse_condition(const std::string &effect_name, json_t *root)
{
    if (!root) {
        return nullptr;
    }

    auto type = json_typeof(root);
    switch (type) {
        case JSON_ARRAY: {
            return parse_condition_array(effect_name, root);
        }

        case JSON_OBJECT: {
            return parse_condition_node(effect_name, root);
        }

        default: {
            log_error("Invalid 'if'. Conditions must be a 3 values array, or an object ");
            return nullptr;
        }
    }
}
