/******************************************************************************
    Copyright (C) 2023 by xurei <xureilab@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <string>
#include <vector>
#include <list>
#include <map>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#define QT_UTF8(str) QString::fromUtf8(str, -1)

#include <util/platform.h>
#include <QDoubleSpinBox>
#include "version.h"
#include "logging_functions.hpp"
#include "is_module_loaded.h"

#ifdef DEV_MODE
#include "util/enum_util.hpp"
#include "util/debug_util.hpp"
#endif
#include "util/file_util.h"
#include "util/time_util.hpp"
#include "shader/shaders_library.h"
#include "effect.h"

#include "settings.h"
#include "shadertastic.hpp"
#include "face_tracking/face_tracking.h"
//----------------------------------------------------------------------------------------------------------------------

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("xurei")
OBS_MODULE_USE_DEFAULT_LOCALE("shadertastic", "en-US")
//----------------------------------------------------------------------------------------------------------------------

bool module_loaded = false;
//----------------------------------------------------------------------------------------------------------------------

void load_effects(shadertastic_common *s, obs_data_t *settings, const std::string &effects_dir, const std::string &effects_type) {
    UNUSED_PARAMETER(settings);
    std::vector<std::string> dirs = list_directories((effects_dir + "/" + effects_type + "s").c_str());

    for (const auto &dir : dirs) {
        std::string effect_path = std::string(effects_dir)
            .append("/")
            .append(effects_type)
            .append("s/")
            .append(dir);
        if (s->effects->find(dir) != s->effects->end()) {
            warn("NOT LOADING EFFECT %s/%ss/%s : an effect with the name '%s' already exist", effects_dir.c_str(), effects_type.c_str(), dir.c_str(), dir.c_str());
        }
        else if (os_file_exists((effect_path + "/meta.json").c_str())) {
            debug("Effect %s", effect_path.c_str());
            shadertastic_effect_t effect(dir, effect_path);
            effect.load();
            if (effect.main_shader == nullptr) {
                debug ("NOT LOADING EFFECT %s", dir.c_str());
            }
            else {
                s->effects->insert(shadertastic_effects_map_t::value_type(dir, effect));

                // Defaults must be set here and not in the transition_defaults() function.
                // as the effects are not loaded yet in transition_defaults()
                for (auto param: effect.effect_params) {
                    std::string full_param_name = param->get_full_param_name(effect.name.c_str());
                    param->set_default(settings, full_param_name.c_str());
                }
            }
        }
        else {
            debug ("NOT LOADING EFFECT %s : no meta.json found", dir.c_str());
        }
    }

    std::string extension = std::string(".") + effects_type + ".shadertastic";
    std::vector<std::string> zips = list_files(effects_dir.c_str(), extension);
    for (const auto &zip : zips) {
        fs::path fs_path(zip);
        std::string effect_name = fs_path.filename().string();
        effect_name = effect_name.substr(0, effect_name.length() - extension.length());
        std::string effect_path = zip;

        if (s->effects->find(effect_name) != s->effects->end()) {
            warn("NOT LOADING EFFECT %s/%s : an effect with the name '%s' already exist", effects_dir.c_str(), zip.c_str(), effect_name.c_str());
        }
        else if (true /*shadertastic_effect_t::is_effect(effect_path)*/) {
            // TODO the check that meta.json exists is missing in archived effects... I should add it back
            debug("Effect %s: %s", effect_name.c_str(), effect_path.c_str());
            shadertastic_effect_t effect(effect_name, effect_path);
            effect.load();
            if (effect.main_shader == nullptr) {
                debug ("NOT LOADING EFFECT %s", zip.c_str());
            }
            else {
                s->effects->insert(shadertastic_effects_map_t::value_type(effect_name, effect));

                // Defaults must be set here and not in the transition_defaults() function.
                // as the effects are not loaded yet in transition_defaults()
                for (auto param: effect.effect_params) {
                    std::string full_param_name = param->get_full_param_name(effect.name.c_str());
                    param->set_default(settings, full_param_name.c_str());
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

#include "shader_filter.hpp"
#include "shader_transition.hpp"
//----------------------------------------------------------------------------------------------------------------------

#ifndef _WIN32
#pragma clang diagnostic push
#pragma ide diagnostic ignored "MemoryLeak"
#endif
static QDoubleSpinBox * settings_dialog__float_input(QDialog *dialog, QFormLayout* layout, std::string input_label, std::string comments, float value, float step, const float min_val, const float max_val) {
    QHBoxLayout *inputLayout = new QHBoxLayout;

    QLabel *label = new QLabel(QString((input_label + " (" + comments + ")").c_str()), dialog);
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(dialog);

    // Set a placeholder and a double validator for the input
    spinBox->setRange(0, 9999.0);
    spinBox->setDecimals(4);
    spinBox->setSingleStep(step);
    spinBox->setValue(value);
    spinBox->setMinimum(min_val);
    spinBox->setMaximum(max_val);

    // Add widgets to the input layout
    inputLayout->addWidget(label);
    inputLayout->addWidget(spinBox);

    // Add the input layout to the main layout
    layout->addRow(inputLayout);

    return spinBox;
}
#ifndef _WIN32
#pragma clang diagnostic pop
#endif

#ifndef _WIN32
#pragma clang diagnostic push
#pragma ide diagnostic ignored "MemoryLeak"
#endif
static void show_settings_dialog() {
    obs_data_t *settings = load_settings();

    // Create the settings dialog
    QDialog *dialog = new QDialog();
    dialog->setWindowTitle("Shadertastic");
    dialog->setFixedSize(dialog->sizeHint());
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(20, 12, 20, 12);
    QFormLayout *formLayout = new QFormLayout();

    // External folder input
    QHBoxLayout* layout = new QHBoxLayout();
    QLabel *effectsLabel = new QLabel("Additional effects path");
    formLayout->addRow(effectsLabel);
    QLineEdit *filePathLineEdit = new QLineEdit();
    filePathLineEdit->setReadOnly(true);
    filePathLineEdit->setText(QT_UTF8(obs_data_get_string(settings, SETTING_EFFECTS_PATH)));
    layout->addWidget(filePathLineEdit);
    QPushButton *filePickerButton = new QPushButton("Select folder...");
    QObject::connect(filePickerButton, &QPushButton::clicked, [=](){
        QString selectedFolder = QFileDialog::getExistingDirectory();
        if (!selectedFolder.isEmpty()) {
            const char *selectedFolderStr = selectedFolder.toUtf8().constData();
            debug("%s = %s", SETTING_EFFECTS_PATH, selectedFolderStr);
            obs_data_set_string(settings, SETTING_EFFECTS_PATH, selectedFolderStr);
            filePathLineEdit->setText(selectedFolder);
        }
        else {
            obs_data_set_string(settings, SETTING_EFFECTS_PATH, nullptr);
            filePathLineEdit->setText("");
        }
    });
    layout->addWidget(filePickerButton);
    formLayout->addRow(layout);

    formLayout->addRow(new QLabel(
        "Place your own effects in this folder to load them.\n"
        "Two subfolders will be created: 'filters' and 'transitions', one for each type of effect."
    ));
    QLabel *effectsLabelDescription = new QLabel(
        "You can find more effects in the <a href=\"https://shadertastic.com/library\">Shadertastic library</a>"
    );
    effectsLabelDescription->setOpenExternalLinks(true);
    formLayout->addRow(effectsLabelDescription);

    // Separator
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(0);
    formLayout->addRow(line);

    // Developper mode
    {
        QCheckBox *devmodeCheckbox = new QCheckBox("Developper mode");
        devmodeCheckbox->setChecked(obs_data_get_bool(settings, SETTING_DEV_MODE_ENABLED));
        QObject::connect(devmodeCheckbox, &QCheckBox::clicked, [=]() {
            bool checked = devmodeCheckbox->isChecked();
            obs_data_set_bool(settings, SETTING_DEV_MODE_ENABLED, checked);
        });
        formLayout->addRow(devmodeCheckbox);
        formLayout->addRow(new QLabel(
            "WARNING: this has a great impact on stability and performance.\n"
            "Enable this if you are creating your own effects and allow to hot-reload them."
        ));
    }

    // Separator
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(0);
    formLayout->addRow(line);

    // One Euro Filter params
    {
        QCheckBox *oneEuroEnable = new QCheckBox("Face tracking smoothing");
        oneEuroEnable->setChecked(obs_data_get_bool(settings, SETTING_ONE_EURO_ENABLED));
        QObject::connect(oneEuroEnable, &QCheckBox::clicked, [=]() {
            bool checked = oneEuroEnable->isChecked();
            obs_data_set_bool(settings, SETTING_ONE_EURO_ENABLED, checked);
            apply_settings(settings);
        });
        formLayout->addRow(oneEuroEnable);
    }

    QHBoxLayout *infosLayout = new QHBoxLayout;
    QLabel *infosLabel = new QLabel("Apply a smoothing effect on the face tracking feature, using the <a href=\"https://gery.casiez.net/1euro/\">1€ Filter</a>.<br>Disable this if you want the most precise detection, but some effects will be wobbly.");
    infosLabel->setOpenExternalLinks(true);
    infosLayout->addWidget(infosLabel);
    formLayout->addRow(infosLayout);

    QDoubleSpinBox *one_euro_min_cutoff_edit;
    {
        float one_euro_filter_mincutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_MIN_CUTOFF);
        one_euro_min_cutoff_edit = settings_dialog__float_input(dialog, formLayout, "Min Cutoff", "Lower is smoother", one_euro_filter_mincutoff, 0.000f, 0.0f, 20.0f);
        QObject::connect(one_euro_min_cutoff_edit, &QDoubleSpinBox::textChanged, [=]() {
            float float_value = (float)(one_euro_min_cutoff_edit->value());
            obs_data_set_double(settings, SETTING_ONE_EURO_MIN_CUTOFF, float_value);
            apply_settings(settings);
        });
    }

    QDoubleSpinBox *one_euro_beta_edit;
    {
        float one_euro_filter_beta = (float)obs_data_get_double(settings, SETTING_ONE_EURO_BETA);
        one_euro_beta_edit = settings_dialog__float_input(dialog, formLayout, "Beta", "Lower is smoother", one_euro_filter_beta, 0.1f, 0.0f, 10000.0f);
        QObject::connect(one_euro_beta_edit, &QDoubleSpinBox::textChanged, [=]() {
            float float_value = (float)(one_euro_beta_edit->value());
            obs_data_set_double(settings, SETTING_ONE_EURO_BETA, float_value);
            apply_settings(settings);
        });
    }

//    QDoubleSpinBox *one_euro_derivcutoff_edit;
//    {
//        float one_euro_filter_derivcutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF);
//        one_euro_derivcutoff_edit = settings_dialog__float_input(dialog, formLayout, "Deriv Cutoff", "Lower is smoother", one_euro_filter_derivcutoff, 0.0001f, 0.0f, 20.0f);
//        QObject::connect(one_euro_derivcutoff_edit, &QDoubleSpinBox::textChanged, [=]() {
//            float float_value = (float)(one_euro_derivcutoff_edit->value());
//            obs_data_set_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF, float_value);
//            apply_settings(settings);
//        });
//    }

    {
        QPushButton *oneEuroDefaultsButton = new QPushButton("Defaults");
        QObject::connect(oneEuroDefaultsButton, &QPushButton::clicked, [=]() {
            one_euro_min_cutoff_edit->setValue(obs_data_get_default_double(settings, SETTING_ONE_EURO_MIN_CUTOFF));
            one_euro_beta_edit->setValue(obs_data_get_default_double(settings, SETTING_ONE_EURO_BETA));
            //one_euro_derivcutoff_edit->setValue(obs_data_get_default_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF));
            apply_settings(settings);
        });
        QHBoxLayout *defaultsLayout = new QHBoxLayout;
        QLabel *spacer = new QLabel(QString(""));
        defaultsLayout->addWidget(spacer);
        defaultsLayout->addWidget(oneEuroDefaultsButton);
        oneEuroDefaultsButton->setMaximumWidth(100);
        formLayout->addRow(defaultsLayout);
    }

    // Separator
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(0);
    formLayout->addRow(line);

    // OK & Cancel Buttons
    layout = new QHBoxLayout();
    layout->addStretch();
    formLayout->addRow(layout);

    QPushButton *cancelButton = new QPushButton("Cancel");
    QObject::connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::close);
    layout->addWidget(cancelButton);

    QPushButton *saveButton = new QPushButton("Save");
    QObject::connect(saveButton, &QPushButton::clicked, [=]() {
        apply_settings(settings);
        save_settings(settings);
        dialog->close();
    });
    layout->addWidget(saveButton);

    // About
    layout = new QHBoxLayout();
    layout->addStretch();
    QLabel *aboutLabel = new QLabel("Version " PROJECT_VERSION " by <a href=\"http://about.xurei.io/\">xurei</a>");
    aboutLabel->setOpenExternalLinks(true);
    layout->addWidget(aboutLabel);
    formLayout->addRow(layout);

    // Build ref
    layout = new QHBoxLayout();
    layout->addStretch();
    QLabel *buildLabel = new QLabel("Build: " PROJECT_VERSION_COMMIT "");
    layout->addWidget(buildLabel);
    formLayout->addRow(layout);

    mainLayout->addLayout(formLayout);

    QObject::connect(dialog, &QDialog::finished, [=](int) {
        obs_data_release(settings);
    });

    dialog->show();
}
#ifndef _WIN32
#pragma clang diagnostic pop
#endif
//----------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] bool obs_module_load(void) {
    info("loaded version %s", PROJECT_VERSION);
    srand(time(nullptr));
    obs_data_t *settings = load_settings();
    apply_settings(settings);
    obs_data_release(settings);

    struct obs_source_info shadertastic_transition_info = {};
    shadertastic_transition_info.id =
        #ifdef DEV_MODE
          "shadertastic_transition_dev";
        #else
          "shadertastic_transition";
        #endif

    shaders_library.load();

    shadertastic_transition_info.type = OBS_SOURCE_TYPE_TRANSITION;
    shadertastic_transition_info.get_name = shadertastic_transition_get_name;
    shadertastic_transition_info.create = shadertastic_transition_create;
    shadertastic_transition_info.destroy = shadertastic_transition_destroy;
    shadertastic_transition_info.get_properties = shadertastic_transition_properties;
    shadertastic_transition_info.update = shadertastic_transition_update;
    shadertastic_transition_info.video_render = shadertastic_transition_video_render;
    shadertastic_transition_info.load = shadertastic_transition_update;
    shadertastic_transition_info.audio_render = shadertastic_transition_audio_render;
    shadertastic_transition_info.transition_start = shadertastic_transition_start;
    shadertastic_transition_info.transition_stop = shadertastic_transition_stop;
    shadertastic_transition_info.get_defaults2 = shadertastic_transition_defaults;
    //shadertastic_transition_info.video_tick = shadertastic_transition_tick;
    shadertastic_transition_info.video_get_color_space = shadertastic_transition_get_color_space;
    obs_register_source(&shadertastic_transition_info);

    struct obs_source_info shadertastic_filter_info = {};
    shadertastic_filter_info.id = "shadertastic_filter";
    shadertastic_filter_info.type = OBS_SOURCE_TYPE_FILTER;
    shadertastic_filter_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW /*| OBS_SOURCE_COMPOSITE*/;
    shadertastic_filter_info.get_name = shadertastic_filter_get_name;
    shadertastic_filter_info.create = shadertastic_filter_create;
    shadertastic_filter_info.destroy = shadertastic_filter_destroy;
    shadertastic_filter_info.get_properties = shadertastic_filter_properties;
    shadertastic_filter_info.video_tick = shadertastic_filter_tick;
    shadertastic_filter_info.update = shadertastic_filter_update;
    shadertastic_filter_info.get_width = shadertastic_filter_getwidth,
    shadertastic_filter_info.get_height = shadertastic_filter_getheight,
    shadertastic_filter_info.video_render = shadertastic_filter_video_render;
    shadertastic_filter_info.load = shadertastic_filter_update;
    shadertastic_filter_info.show = shadertastic_filter_show;
    shadertastic_filter_info.hide = shadertastic_filter_hide;
    shadertastic_filter_info.video_get_color_space = shadertastic_filter_get_color_space;
    obs_register_source(&shadertastic_filter_info);

    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(obs_module_text("Shadertastic Settings")));
    QObject::connect(action, &QAction::triggered, [] { show_settings_dialog(); });

    module_loaded = true;
    return true;
}
//----------------------------------------------------------------------------------------------------------------------

bool is_module_loaded() {
    return module_loaded;
}

[[maybe_unused]] void obs_module_unload(void) {
    module_loaded = false;
}
//----------------------------------------------------------------------------------------------------------------------
