#include "shadertastic_common.hpp"

void shadertastic_common::release() {
    if (effects != nullptr) {
        for (auto &[key, effect]: *effects) {
            effect.release();
        }
        effects->clear();
    }
    release_resource(delete, effects);
}
//----------------------------------------------------------------------------------------------------------------------
