#ifndef SHADERTASTIC_SETTINGS_H
#define SHADERTASTIC_SETTINGS_H

#define SETTING_EFFECTS_PATH "effects_path"
#define SETTING_DEV_MODE_ENABLED "dev_mode_enabled"
#define SETTING_ONE_EURO_ENABLED "one_euro_enabled"
#define SETTING_ONE_EURO_MIN_CUTOFF "one_euro_min_cutoff"
#define SETTING_ONE_EURO_BETA "one_euro_beta"
#define SETTING_ONE_EURO_DERIV_CUTOFF "one_euro_deriv_cutoff"

#include <string>

struct shadertastic_settings_t {
    // Path where the user can add their own effects
    std::string *effects_path = nullptr;
    bool dev_mode_enabled = false;
    bool one_euro_enabled = false;
    float one_euro_min_cutoff = 10.0f;
    float one_euro_deriv_cutoff = 0.007f;
    float one_euro_beta = 10.0f;
};
//----------------------------------------------------------------------------------------------------------------------

obs_data_t * load_settings();

void save_settings(obs_data_t *settings);

void apply_settings(obs_data_t *settings);

const shadertastic_settings_t & shadertastic_settings();

#endif /* SHADERTASTIC_SETTINGS_H */
