#ifndef SHADERTASTIC_OBS_PROPERTY_ADD_MODIFIED_CALLBACK2_HPP
#define SHADERTASTIC_OBS_PROPERTY_ADD_MODIFIED_CALLBACK2_HPP

#include <obs-properties.h>

uint64_t obs_property_add_modified_callback2(obs_property_t *p, obs_property_modified2_t cb, void *data);

void obs_property_remove_modified_callback2(obs_property_t *p, uint64_t id);

#endif  // SHADERTASTIC_OBS_PROPERTY_ADD_MODIFIED_CALLBACK2_HPP
