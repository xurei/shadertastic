#include <vector>
#include <map>
#include <algorithm>
#include "./obs_property_add_modified_callback2.h"

struct obs_modified_callback_entry {
    uint64_t id;
    obs_property_modified2_t cb;
    void *data;
};

struct obs_modified_callback_group {
    std::vector<obs_modified_callback_entry> callbacks;
    uint64_t next_id = 1;
};

static std::map<obs_property_t*, obs_modified_callback_group> callback_groups;

static obs_modified_callback_group* get_callback_group(obs_property_t *p) {
    if (callback_groups.find(p) == callback_groups.end()) {
        callback_groups[p].next_id = 0;
    }

    return &callback_groups[p];
}

static bool obs_modified_callback_dispatch(void *data, obs_properties_t *props, obs_property_t *property, obs_data_t *settings) {
    auto *group = static_cast<obs_modified_callback_group *>(data);

    bool result = false;

    for (auto &entry : group->callbacks) {
        if (entry.cb) {
            bool r = entry.cb(entry.data, props, property, settings);
            result = result || r;
        }
    }

    return result;
}

uint64_t obs_property_add_modified_callback2(obs_property_t *p, obs_property_modified2_t cb, void *data) {
    auto group = get_callback_group(p);
    uint64_t id = group->next_id++;
    group->callbacks.push_back({
        id,
        cb,
        data
    });

    // on (re)set le callback unique côté OBS
    obs_property_set_modified_callback2(
        p,
        obs_modified_callback_dispatch,
        group
    );
    return id;
}

void obs_property_remove_modified_callback2(obs_property_t *p, uint64_t id) {
    auto group = get_callback_group(p);
    auto &vec = group->callbacks;

    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [id](const obs_modified_callback_entry &e) {
                return e.id == id;
            }),
        vec.end()
    );

    if (vec.empty()) {
        obs_property_set_modified_callback2(p, nullptr, nullptr);
        return;
    }

    // rebind dispatcher (toujours safe, OBS écrase le callback sinon)
    obs_property_set_modified_callback2(
        p,
        obs_modified_callback_dispatch,
        group
    );
}
