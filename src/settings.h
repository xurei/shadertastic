#ifndef SHADERTASTIC_SETTINGS_H
#define SHADERTASTIC_SETTINGS_H

#define SETTING_EFFECTS_PATH "effects_path"
#define SETTING_EFFECTS_PATHS "effects_paths"
#define SETTING_DEV_MODE_ENABLED "dev_mode_enabled"
#define SETTING_DEV_MODE_LIBRARY_PATH "dev_mode_library_path"
#define SETTING_ONE_EURO_ENABLED "one_euro_enabled"
#define SETTING_ONE_EURO_MIN_CUTOFF "one_euro_min_cutoff"
#define SETTING_ONE_EURO_BETA "one_euro_beta"
#define SETTING_ONE_EURO_DERIV_CUTOFF "one_euro_deriv_cutoff"

#include <string>
#include <vector>

struct shadertastic_settings_t {
    // Path where the user can add their own effects
    std::vector<std::string> effects_paths;
    std::string library_path;
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
void apply_settings__facetracking(obs_data_t *settings);

const shadertastic_settings_t & shadertastic_settings();

void show_settings_dialog();

#endif // SHADERTASTIC_SETTINGS_H
