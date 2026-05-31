//
// Created by olivier on 5/30/26.
//

#ifndef SHADERTASTIC_CONDITION_PARSER_H
#define SHADERTASTIC_CONDITION_PARSER_H

#include <string>
#include <memory>
#include <obs-data.h>
#include <jansson.h>
#include "condition.hpp"

std::unique_ptr<condition_t> parse_condition(const std::string &effect_name, json_t *node);

#endif //SHADERTASTIC_CONDITION_PARSER_H
