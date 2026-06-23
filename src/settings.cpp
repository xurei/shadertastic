#include <obs-module.h>
#include <util/platform.h>
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QStringList>
#include <QVBoxLayout>

#include "version.h"
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

    obs_data_set_default_bool(settings, SETTING_ONE_EURO_ENABLED, true);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_MIN_CUTOFF, 0.0);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_BETA, 32.0);
    obs_data_set_default_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF, 1.0);

    if (!settings) {
        info("Settings not found. Creating default settings in %s ...", file);
        char *obs_base_config_path = obs_module_config_path("");
        os_mkdirs(obs_base_config_path);
        bfree(obs_base_config_path);
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

    // Migration - effects path
    const char *effects_paths_str = obs_data_get_string(settings, SETTING_EFFECTS_PATH);
    if (effects_paths_str != nullptr && strlen(effects_paths_str) > 0) {
        obs_data_array_t *effects_paths_array = obs_data_get_array(settings, SETTING_EFFECTS_PATHS);
        if (effects_paths_array == nullptr) {
            effects_paths_array = obs_data_array_create();
        }
        obs_data_t *path_obj = obs_data_create();
        obs_data_set_string(path_obj, "path", effects_paths_str);
        obs_data_array_push_back(effects_paths_array, path_obj);
        obs_data_release(path_obj);
        obs_data_set_array(settings, SETTING_EFFECTS_PATHS, effects_paths_array);
        obs_data_erase(settings, SETTING_EFFECTS_PATH);
    }

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
    shadertastic_settings_.effects_paths.clear();
    obs_data_array_t *effects_paths_array = obs_data_get_array(settings, SETTING_EFFECTS_PATHS);
    if (effects_paths_array != nullptr) {
        size_t count = obs_data_array_count(effects_paths_array);
        for (size_t i = 0; i < count; i++) {
            obs_data_t *item = obs_data_array_item(effects_paths_array, i);
            const char *path = obs_data_get_string(item, "path");
            if (path != nullptr) {
                shadertastic_settings_.effects_paths.push_back(std::string(path));
            }
            obs_data_release(item);
        }
        obs_data_array_release(effects_paths_array);
    }
    shadertastic_settings_.dev_mode_enabled = obs_data_get_bool(settings, SETTING_DEV_MODE_ENABLED);

    // This is called in a separate function because changing them in the UI will immediately update the settings
    apply_settings__facetracking(settings);
}
//----------------------------------------------------------------------------------------------------------------------

void apply_settings__facetracking(obs_data_t *settings) {
    shadertastic_settings_.one_euro_enabled = obs_data_get_bool(settings, SETTING_ONE_EURO_ENABLED);
    shadertastic_settings_.one_euro_min_cutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_MIN_CUTOFF);
    shadertastic_settings_.one_euro_deriv_cutoff = (float)obs_data_get_double(settings, SETTING_ONE_EURO_DERIV_CUTOFF);
    shadertastic_settings_.one_euro_beta = (float)obs_data_get_double(settings, SETTING_ONE_EURO_BETA);
}
//----------------------------------------------------------------------------------------------------------------------

const shadertastic_settings_t & shadertastic_settings() {
    return shadertastic_settings_;
}
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

static void save_path_list(obs_data_t *settings, QVBoxLayout *pathsList) {
    // Update settings with all paths
    obs_data_array_t *paths_array = obs_data_array_create();
    for (int i = 0; i < pathsList->count(); i++) {
        QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(pathsList->itemAt(i)->layout());
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(layout->itemAt(0)->widget());
        if (lineEdit && !lineEdit->text().isEmpty()) {
            obs_data_t *path_obj = obs_data_create();
            obs_data_set_string(path_obj, "path", lineEdit->text().toUtf8().constData());
            obs_data_array_push_back(paths_array, path_obj);
            obs_data_release(path_obj);
        }
    }

    obs_data_set_array(settings, SETTING_EFFECTS_PATHS, paths_array);
    debug("%s", obs_data_get_json(settings));
    obs_data_array_release(paths_array);
}

static void add_path_line(obs_data_t *settings, QVBoxLayout *pathsList, const QString &path) {
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLineEdit *pathLineEdit = new QLineEdit();
    pathLineEdit->setReadOnly(true);
    pathLineEdit->setText(path);
    pathLayout->addWidget(pathLineEdit);

    QPushButton *changeButton = new QPushButton("Select...");
    QObject::connect(changeButton, &QPushButton::clicked, [=]() {
        QString oldPath = pathLineEdit->text();

        QString selectedFolder = QFileDialog::getExistingDirectory(nullptr, "Select folder", oldPath);
        if (!selectedFolder.isEmpty()) {
            pathLineEdit->setText(selectedFolder);
            save_path_list(settings, pathsList);
        }
    });
    pathLayout->addWidget(changeButton);

    QPushButton *removeButton = new QPushButton("");
    removeButton->setProperty("class", QString("icon-trash"));
    pathLayout->addWidget(removeButton);

    QObject::connect(removeButton, &QPushButton::clicked, [=]() {
        QLayoutItem *item = pathsList->takeAt(pathsList->indexOf(pathLayout));
        delete item->widget();
        delete item;

        // Update settings with remaining paths
        save_path_list(settings, pathsList);
    });

    pathsList->addLayout(pathLayout);
}

void show_settings_dialog() {
    obs_data_t *settings = load_settings();

    // Create the settings dialog
    QDialog *dialog = new QDialog();
    dialog->setWindowTitle("Shadertastic");
    dialog->setFixedSize(dialog->sizeHint());
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(20, 12, 20, 12);
    QFormLayout *formLayout = new QFormLayout();

    QHBoxLayout* layout;

    // External folder inputs
    {
        QVBoxLayout *pathsLayout = new QVBoxLayout();
        QLabel *effectsLabel = new QLabel("Additional effects folders");
        formLayout->addRow(effectsLabel);

        formLayout->addRow(new QLabel(
            "Place your own effects in this folder to load them."
        ));
        QLabel *effectsLabelDescription = new QLabel(
            "You can find more effects in the <a href=\"https://shadertastic.com/library\">Shadertastic library</a>"
        );
        effectsLabelDescription->setOpenExternalLinks(true);
        formLayout->addRow(effectsLabelDescription);

        QWidget *pathsWidget = new QWidget();
        QVBoxLayout *pathsList = new QVBoxLayout(pathsWidget);
        pathsList->setContentsMargins(0, 0, 0, 0);

        obs_data_array_t *paths_array = obs_data_get_array(settings, SETTING_EFFECTS_PATHS);
        QStringList paths;
        if (paths_array != nullptr) {
            size_t count = obs_data_array_count(paths_array);
            for (size_t i = 0; i < count; i++) {
                obs_data_t *path_obj = obs_data_array_item(paths_array, i);
                const char *path = obs_data_get_string(path_obj, "path");
                paths.append(QString::fromUtf8(path));
                obs_data_release(path_obj);
            }
            obs_data_array_release(paths_array);
        }

        for (const QString &path: paths) {
            add_path_line(settings, pathsList, path);
        }

        pathsLayout->addWidget(pathsWidget);

        QPushButton *addPathButton = new QPushButton("Add folder");
        addPathButton->setProperty("class", QString("icon-plus"));
        QObject::connect(addPathButton, &QPushButton::clicked, [=]() {
            QString selectedFolder = QFileDialog::getExistingDirectory(nullptr, "Select folder", QDir::homePath());
            if (!selectedFolder.isEmpty()) {
                add_path_line(settings, pathsList, selectedFolder);
                save_path_list(settings, pathsList);
            }
        });

        QHBoxLayout *addPathLayout = new QHBoxLayout();
        addPathLayout->addStretch(1);
        addPathLayout->addWidget(addPathButton);
        pathsLayout->addLayout(addPathLayout);
        formLayout->addRow(pathsLayout);
    }

    // Separator
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(0);
    formLayout->addRow(line);

    // Developer mode
    {
        QCheckBox *devmodeCheckbox = new QCheckBox("Developer mode");
        devmodeCheckbox->setChecked(obs_data_get_bool(settings, SETTING_DEV_MODE_ENABLED));
        QObject::connect(devmodeCheckbox, &QCheckBox::clicked, [=]() {
            bool checked = devmodeCheckbox->isChecked();
            obs_data_set_bool(settings, SETTING_DEV_MODE_ENABLED, checked);
        });
        formLayout->addRow(devmodeCheckbox);
        formLayout->addRow(new QLabel(
            "WARNING: this has a great impact on stability and performance.\n"
            "Enable this if you are creating your own effects and want to hot-reload them."
        ));
        QLabel *effectsLabelDescription2 = new QLabel(
            "Create your own effects: <a href=\"https://doc.shadertastic.com/effect-development/getting-started/\">Documentation</a> (WORK IN PROGRESS)"
        );
        effectsLabelDescription2->setOpenExternalLinks(true);
        formLayout->addRow(effectsLabelDescription2);
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
            apply_settings__facetracking(settings);
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
        one_euro_min_cutoff_edit = settings_dialog__float_input(dialog, formLayout, "Min Cutoff", "Lower is smoother", one_euro_filter_mincutoff, 0.0001f, 0.0f, 20.0f);
        QObject::connect(one_euro_min_cutoff_edit, &QDoubleSpinBox::textChanged, [=]() {
            float float_value = (float)(one_euro_min_cutoff_edit->value());
            obs_data_set_double(settings, SETTING_ONE_EURO_MIN_CUTOFF, float_value);
            apply_settings__facetracking(settings);
        });
    }

    QDoubleSpinBox *one_euro_beta_edit;
    {
        float one_euro_filter_beta = (float)obs_data_get_double(settings, SETTING_ONE_EURO_BETA);
        one_euro_beta_edit = settings_dialog__float_input(dialog, formLayout, "Beta", "Lower is smoother", one_euro_filter_beta, 0.1f, 0.0f, 10000.0f);
        QObject::connect(one_euro_beta_edit, &QDoubleSpinBox::textChanged, [=]() {
            float float_value = (float)(one_euro_beta_edit->value());
            obs_data_set_double(settings, SETTING_ONE_EURO_BETA, float_value);
            apply_settings__facetracking(settings);
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
            apply_settings__facetracking(settings);
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
