#ifndef SHADERTASTIC_TRY_GS_EFFECT_SET_H
#define SHADERTASTIC_TRY_GS_EFFECT_SET_H

#include "logging_functions.hpp"
#include <unistd.h>

// Uncomment this and comment the duplicate #define to print the param assignation (this will spam the logs)
//#define debug_try_gs_effect(format, ...) debug(format, ##__VA_ARGS__); usleep(100)
#define debug_try_gs_effect(format, ...)

#define try_gs_effect_set_val(param_name, param, data, data_size) if (param) { debug_try_gs_effect("try_gs_effect_set_val %s", param_name); gs_effect_set_val(param, data, data_size); debug_try_gs_effect("try_gs_effect_set_val %s DONE", param_name); }
#define try_gs_effect_set_texture(param_name, param, val) if (param)         { debug_try_gs_effect("try_gs_effect_set_texture %s", param_name); gs_effect_set_texture(param, val); debug_try_gs_effect("try_gs_effect_set_texture %s DONE", param_name); }
#define try_gs_effect_set_texture_srgb(param_name, param, val) if (param)    { debug_try_gs_effect("try_gs_effect_set_texture_srgb %s", param_name); gs_effect_set_texture_srgb(param, val); debug_try_gs_effect("try_gs_effect_set_texture_srgb %s DONE", param_name); }
#define try_gs_effect_set_int(param_name, param, val) if (param)             { debug_try_gs_effect("try_gs_effect_set_int %s", param_name); gs_effect_set_int(param, val); debug_try_gs_effect("try_gs_effect_set_int %s DONE", param_name); }
#define try_gs_effect_set_float(param_name, param, val) if (param)           { debug_try_gs_effect("try_gs_effect_set_float %s", param_name); gs_effect_set_float(param, val); debug_try_gs_effect("try_gs_effect_set_float %s DONE", param_name); }
#define try_gs_effect_set_vec2(param_name, param, val) if (param)            { debug_try_gs_effect("try_gs_effect_set_vec2 %s", param_name); gs_effect_set_vec2(param, val); debug_try_gs_effect("try_gs_effect_set_vec2 %s DONE", param_name); }
#define try_gs_effect_set_bool(param_name, param, val) if (param)            { debug_try_gs_effect("try_gs_effect_set_bool %s", param_name); gs_effect_set_bool(param, val); debug_try_gs_effect("try_gs_effect_set_bool %s DONE", param_name); }

#endif /* SHADERTASTIC_TRY_GS_EFFECT_SET_H */
