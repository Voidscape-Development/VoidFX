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

#pragma once

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Per-frame information handed to an effect right before it is drawn. */
struct voidfx_frame {
	uint32_t width;
	uint32_t height;
	/* Seconds since the filter was created, used by animated effects. */
	float time;
};

/*
 * A single visual effect selectable from the VoidFX filter's dropdown.
 *
 * Every effect owns a property group; the filter shows only the group of the
 * currently selected effect. Setting names must be globally unique, so each
 * effect prefixes its settings (e.g. "ca_", "sg_").
 */
struct voidfx_effect_info {
	/* Stable identifier stored in the filter settings. Never change it. */
	const char *id;
	/* Locale key for the dropdown entry and property group label. */
	const char *name_key;
	/* Name of the property group holding this effect's settings. */
	const char *group_name;
	/* Effect file relative to the plugin's data directory. */
	const char *effect_file;

	/* Called inside the graphics context after the effect file is loaded. */
	void *(*create)(gs_effect_t *effect);
	void (*destroy)(void *data);

	void (*get_defaults)(obs_data_t *settings);
	void (*add_properties)(obs_properties_t *group);
	void (*update)(void *data, obs_data_t *settings);

	/* Uploads the effect's parameters; the filter handles the draw. */
	void (*set_params)(void *data, const struct voidfx_frame *frame);
};

extern const struct voidfx_effect_info voidfx_chromatic_aberration;
extern const struct voidfx_effect_info voidfx_shattered_glass;

#ifdef __cplusplus
}
#endif
