/*
VoidFX
Copyright (C) 2026 Voidscape Development

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

#include "voidfx-effect.h"

#define SETTING_EFFECT_TYPE "effect_type"

/* Order here is the order shown in the dropdown. */
static const struct voidfx_effect_info *const voidfx_effects[] = {
	&voidfx_chromatic_aberration,
	&voidfx_shattered_glass,
};

#define VOIDFX_EFFECT_COUNT (sizeof(voidfx_effects) / sizeof(voidfx_effects[0]))

struct voidfx_effect_instance {
	gs_effect_t *effect;
	void *data;
};

struct voidfx_filter {
	obs_source_t *source;
	struct voidfx_effect_instance instances[VOIDFX_EFFECT_COUNT];
	size_t active;
	float time;
};

static size_t find_effect_index(const char *id)
{
	if (id) {
		for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
			if (strcmp(voidfx_effects[i]->id, id) == 0)
				return i;
		}
	}
	return 0;
}

static const char *voidfx_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("VoidFX");
}

static void voidfx_update(void *data, obs_data_t *settings)
{
	struct voidfx_filter *filter = data;

	filter->active = find_effect_index(obs_data_get_string(settings, SETTING_EFFECT_TYPE));

	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
		struct voidfx_effect_instance *inst = &filter->instances[i];
		if (inst->data)
			voidfx_effects[i]->update(inst->data, settings);
	}
}

static void voidfx_destroy(void *data)
{
	struct voidfx_filter *filter = data;

	obs_enter_graphics();
	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
		struct voidfx_effect_instance *inst = &filter->instances[i];
		if (inst->data)
			voidfx_effects[i]->destroy(inst->data);
		gs_effect_destroy(inst->effect);
	}
	obs_leave_graphics();

	bfree(filter);
}

static void *voidfx_create(obs_data_t *settings, obs_source_t *source)
{
	struct voidfx_filter *filter = bzalloc(sizeof(*filter));
	filter->source = source;

	obs_enter_graphics();
	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
		const struct voidfx_effect_info *info = voidfx_effects[i];
		struct voidfx_effect_instance *inst = &filter->instances[i];

		char *path = obs_module_file(info->effect_file);
		char *errors = NULL;
		inst->effect = path ? gs_effect_create_from_file(path, &errors) : NULL;

		if (inst->effect) {
			inst->data = info->create(inst->effect);
		} else {
			obs_log(LOG_ERROR, "failed to load effect '%s': %s", info->effect_file,
				errors ? errors : "file not found");
		}

		bfree(errors);
		bfree(path);
	}
	obs_leave_graphics();

	voidfx_update(filter, settings);
	return filter;
}

static void voidfx_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_EFFECT_TYPE, voidfx_effects[0]->id);

	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++)
		voidfx_effects[i]->get_defaults(settings);
}

static bool effect_type_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);

	size_t active = find_effect_index(obs_data_get_string(settings, SETTING_EFFECT_TYPE));

	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
		obs_property_t *group = obs_properties_get(props, voidfx_effects[i]->group_name);
		obs_property_set_visible(group, i == active);
	}
	return true;
}

static obs_properties_t *voidfx_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_property_t *type = obs_properties_add_list(props, SETTING_EFFECT_TYPE, obs_module_text("EffectType"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++)
		obs_property_list_add_string(type, obs_module_text(voidfx_effects[i]->name_key), voidfx_effects[i]->id);
	obs_property_set_modified_callback(type, effect_type_modified);

	for (size_t i = 0; i < VOIDFX_EFFECT_COUNT; i++) {
		const struct voidfx_effect_info *info = voidfx_effects[i];
		obs_properties_t *group = obs_properties_create();
		info->add_properties(group);
		obs_properties_add_group(props, info->group_name, obs_module_text(info->name_key), OBS_GROUP_NORMAL,
					 group);
	}

	return props;
}

static void voidfx_video_tick(void *data, float seconds)
{
	struct voidfx_filter *filter = data;

	/* Wrap to keep float precision in the shaders over long sessions. */
	filter->time += seconds;
	if (filter->time > 3600.0f)
		filter->time -= 3600.0f;
}

static void voidfx_video_render(void *data, gs_effect_t *unused)
{
	UNUSED_PARAMETER(unused);

	struct voidfx_filter *filter = data;
	struct voidfx_effect_instance *inst = &filter->instances[filter->active];
	obs_source_t *target = obs_filter_get_target(filter->source);
	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);

	if (!inst->effect || !inst->data || !width || !height) {
		obs_source_skip_video_filter(filter->source);
		return;
	}

	if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING))
		return;

	struct voidfx_frame frame = {
		.width = width,
		.height = height,
		.time = filter->time,
	};
	voidfx_effects[filter->active]->set_params(inst->data, &frame);

	obs_source_process_filter_end(filter->source, inst->effect, width, height);
}

struct obs_source_info voidfx_filter_info = {
	.id = "voidfx_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = voidfx_get_name,
	.create = voidfx_create,
	.destroy = voidfx_destroy,
	.update = voidfx_update,
	.get_defaults = voidfx_get_defaults,
	.get_properties = voidfx_get_properties,
	.video_tick = voidfx_video_tick,
	.video_render = voidfx_video_render,
};
