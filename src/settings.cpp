#include <obs-module.h>
#include <util/platform.h>
#include "settings.h"
#include "logging_functions.hpp"

shadertastic_settings_t shadertastic_settings_;
//----------------------------------------------------------------------------------------------------------------------

obs_data_t * load_settings() {
    char *file = obs_module_config_path("settings.json");
    char *path_abs = os_get_abs_path_ptr(file);
    debug("Settings path: %s", path_abs);

    obs_data_t *settings = obs_data_create_from_json_file(path_abs);
    obs_data_set_default_bool(settings, SETTING_DEV_MODE_ENABLED, false);

    obs_data_set_default_bool(settings, SETTING_ONE_EURO_ENABLED, false);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_MIN_CUTOFF, 5.0);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_BETA, 0.007);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF, 10.0);

    if (!settings) {
        info("Settings not found. Creating default settings in %s ...", file);
        os_mkdirs(obs_module_config_path(""));
        // Create default settings
        settings = obs_data_create();
        if (obs_data_save_json(settings, file)) {
            info("Settings saved to %s", file);
        }
        else {
            warn("Failed to save settings to file.");
        }
    }
    else {
        blog(LOG_INFO, "Settings loaded successfully");
    }
    bfree(file);
    bfree(path_abs);
    return settings;
}
//----------------------------------------------------------------------------------------------------------------------

void save_settings(obs_data_t *settings) {
    char *configPath = obs_module_config_path("settings.json");
    debug("%s", obs_data_get_json(settings));

    if (obs_data_save_json(settings, configPath)) {
        blog(LOG_INFO, "Settings saved to %s", configPath);
    }
    else {
        blog(LOG_WARNING, "Failed to save settings to file.");
    }

    if (configPath != nullptr) {
        bfree(configPath);
    }
}
//----------------------------------------------------------------------------------------------------------------------

void apply_settings(obs_data_t *settings) {
    if (shadertastic_settings_.effects_path != nullptr) {
        delete shadertastic_settings_.effects_path;
    }
    const char *effects_path_str = obs_data_get_string(settings, SETTING_EFFECTS_PATH);
    if (effects_path_str != nullptr) {
        shadertastic_settings_.effects_path = new std::string(effects_path_str);
    }
    else {
        shadertastic_settings_.effects_path = nullptr;
    }

    shadertastic_settings_.one_euro_enabled = obs_data_get_bool(settings, SETTING_ONE_EURO_ENABLED);
    shadertastic_settings_.one_euro_min_cutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_MIN_CUTOFF);
    shadertastic_settings_.one_euro_deriv_cutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF);
    shadertastic_settings_.one_euro_beta = (float)obs_data_get_double(settings, SETTING_ONE_EURO_BETA);

    shadertastic_settings_.dev_mode_enabled = obs_data_get_bool(settings, SETTING_DEV_MODE_ENABLED);
}
//----------------------------------------------------------------------------------------------------------------------

const shadertastic_settings_t & shadertastic_settings() {
    return shadertastic_settings_;
}
//----------------------------------------------------------------------------------------------------------------------
