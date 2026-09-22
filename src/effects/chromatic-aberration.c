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
#include <graphics/vec2.h>
#include <util/bmem.h>
#include <math.h>

#include "../voidfx-effect.h"
#include "fx-util.h"

#define S_MODE "ca_mode"
#define S_STRENGTH "ca_strength"
#define S_ANGLE "ca_angle"
#define S_CENTER_X "ca_center_x"
#define S_CENTER_Y "ca_center_y"
#define S_FALLOFF "ca_falloff"
#define S_JITTER "ca_jitter"
#define S_JITTER_SPEED "ca_jitter_speed"
#define S_SCANLINES "ca_scanlines"
#define S_SCANLINE_SPACING "ca_scanline_spacing"
#define S_NOISE "ca_noise"
#define S_TRACKING "ca_tracking"
#define S_BLEED "ca_bleed"
#define S_WASHED_OUT "ca_washed_out"

enum ca_mode {
	CA_MODE_LINEAR = 0,
	CA_MODE_RADIAL = 1,
};

struct chromatic_aberration {
	gs_effect_t *effect;

	gs_eparam_t *p_uv_size;
	gs_eparam_t *p_time;
	gs_eparam_t *p_radial;
	gs_eparam_t *p_strength;
	gs_eparam_t *p_direction;
	gs_eparam_t *p_center;
	gs_eparam_t *p_falloff;
	gs_eparam_t *p_scanline_intensity;
	gs_eparam_t *p_scanline_spacing;
	gs_eparam_t *p_noise_amount;
	gs_eparam_t *p_tracking_amount;
	gs_eparam_t *p_bleed;
	gs_eparam_t *p_washed_out;

	enum ca_mode mode;
	float strength;
	struct vec2 direction;
	struct vec2 center;
	float falloff;
	float jitter;
	float jitter_speed;
	float scanlines;
	float scanline_spacing;
	float noise;
	float tracking;
	float bleed;
	float washed_out;
};

static void *ca_create(gs_effect_t *effect)
{
	struct chromatic_aberration *ca = bzalloc(sizeof(*ca));
	ca->effect = effect;

	ca->p_uv_size = gs_effect_get_param_by_name(effect, "uv_size");
	ca->p_time = gs_effect_get_param_by_name(effect, "time");
	ca->p_radial = gs_effect_get_param_by_name(effect, "radial");
	ca->p_strength = gs_effect_get_param_by_name(effect, "strength");
	ca->p_direction = gs_effect_get_param_by_name(effect, "direction");
	ca->p_center = gs_effect_get_param_by_name(effect, "center");
	ca->p_falloff = gs_effect_get_param_by_name(effect, "falloff");
	ca->p_scanline_intensity = gs_effect_get_param_by_name(effect, "scanline_intensity");
	ca->p_scanline_spacing = gs_effect_get_param_by_name(effect, "scanline_spacing");
	ca->p_noise_amount = gs_effect_get_param_by_name(effect, "noise_amount");
	ca->p_tracking_amount = gs_effect_get_param_by_name(effect, "tracking_amount");
	ca->p_bleed = gs_effect_get_param_by_name(effect, "bleed");
	ca->p_washed_out = gs_effect_get_param_by_name(effect, "washed_out");

	return ca;
}

static void ca_destroy(void *data)
{
	bfree(data);
}

static void ca_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_MODE, CA_MODE_LINEAR);
	obs_data_set_default_double(settings, S_STRENGTH, 6.0);
	obs_data_set_default_double(settings, S_ANGLE, 0.0);
	obs_data_set_default_double(settings, S_CENTER_X, 50.0);
	obs_data_set_default_double(settings, S_CENTER_Y, 50.0);
	obs_data_set_default_double(settings, S_FALLOFF, 1.5);
	obs_data_set_default_double(settings, S_JITTER, 0.0);
	obs_data_set_default_double(settings, S_JITTER_SPEED, 8.0);
	obs_data_set_default_double(settings, S_SCANLINES, 0.15);
	obs_data_set_default_double(settings, S_SCANLINE_SPACING, 2.0);
	obs_data_set_default_double(settings, S_NOISE, 0.05);
	obs_data_set_default_double(settings, S_TRACKING, 0.0);
	obs_data_set_default_double(settings, S_BLEED, 0.0);
	obs_data_set_default_double(settings, S_WASHED_OUT, 0.0);
}

static bool ca_mode_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);

	bool radial = obs_data_get_int(settings, S_MODE) == CA_MODE_RADIAL;
	obs_property_set_visible(obs_properties_get(props, S_ANGLE), !radial);
	obs_property_set_visible(obs_properties_get(props, S_CENTER_X), radial);
	obs_property_set_visible(obs_properties_get(props, S_CENTER_Y), radial);
	obs_property_set_visible(obs_properties_get(props, S_FALLOFF), radial);
	return true;
}

static void ca_add_properties(obs_properties_t *group)
{
	obs_property_t *p;

	p = obs_properties_add_list(group, S_MODE, obs_module_text("CA.Mode"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("CA.Mode.Linear"), CA_MODE_LINEAR);
	obs_property_list_add_int(p, obs_module_text("CA.Mode.Radial"), CA_MODE_RADIAL);
	obs_property_set_modified_callback(p, ca_mode_modified);

	p = obs_properties_add_float_slider(group, S_STRENGTH, obs_module_text("CA.Strength"), 0.0, 50.0, 0.1);
	obs_property_float_set_suffix(p, " px");

	p = obs_properties_add_float_slider(group, S_ANGLE, obs_module_text("CA.Angle"), 0.0, 360.0, 1.0);
	obs_property_float_set_suffix(p, "°");

	p = obs_properties_add_float_slider(group, S_CENTER_X, obs_module_text("CA.CenterX"), 0.0, 100.0, 0.5);
	obs_property_float_set_suffix(p, "%");
	p = obs_properties_add_float_slider(group, S_CENTER_Y, obs_module_text("CA.CenterY"), 0.0, 100.0, 0.5);
	obs_property_float_set_suffix(p, "%");
	obs_properties_add_float_slider(group, S_FALLOFF, obs_module_text("CA.Falloff"), 0.1, 4.0, 0.05);

	p = obs_properties_add_float_slider(group, S_JITTER, obs_module_text("CA.Jitter"), 0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "%");
	obs_properties_add_float_slider(group, S_JITTER_SPEED, obs_module_text("CA.JitterSpeed"), 0.1, 30.0, 0.1);

	p = obs_properties_add_float_slider(group, S_SCANLINES, obs_module_text("CA.Scanlines"), 0.0, 1.0, 0.01);
	p = obs_properties_add_float_slider(group, S_SCANLINE_SPACING, obs_module_text("CA.ScanlineSpacing"), 1.0, 16.0,
					    0.5);
	obs_property_float_set_suffix(p, " px");
	obs_properties_add_float_slider(group, S_NOISE, obs_module_text("CA.Noise"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(group, S_TRACKING, obs_module_text("CA.Tracking"), 0.0, 1.0, 0.01);
	p = obs_properties_add_float_slider(group, S_BLEED, obs_module_text("CA.Bleed"), 0.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	obs_property_set_long_description(p, obs_module_text("CA.Bleed.Description"));
	p = obs_properties_add_float_slider(group, S_WASHED_OUT, obs_module_text("CA.WashedOut"), 0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "%");
	obs_property_set_long_description(p, obs_module_text("CA.WashedOut.Description"));
}

static void ca_update(void *data, obs_data_t *settings)
{
	struct chromatic_aberration *ca = data;

	ca->mode = (enum ca_mode)obs_data_get_int(settings, S_MODE);
	ca->strength = (float)obs_data_get_double(settings, S_STRENGTH);

	float angle = RAD((float)obs_data_get_double(settings, S_ANGLE));
	vec2_set(&ca->direction, cosf(angle), sinf(angle));

	vec2_set(&ca->center, (float)obs_data_get_double(settings, S_CENTER_X) / 100.0f,
		 (float)obs_data_get_double(settings, S_CENTER_Y) / 100.0f);
	ca->falloff = (float)obs_data_get_double(settings, S_FALLOFF);
	ca->jitter = (float)obs_data_get_double(settings, S_JITTER) / 100.0f;
	ca->jitter_speed = (float)obs_data_get_double(settings, S_JITTER_SPEED);
	ca->scanlines = (float)obs_data_get_double(settings, S_SCANLINES);
	ca->scanline_spacing = (float)obs_data_get_double(settings, S_SCANLINE_SPACING);
	ca->noise = (float)obs_data_get_double(settings, S_NOISE);
	ca->tracking = (float)obs_data_get_double(settings, S_TRACKING);
	ca->bleed = (float)obs_data_get_double(settings, S_BLEED);
	ca->washed_out = (float)obs_data_get_double(settings, S_WASHED_OUT) / 100.0f;
}

static void ca_set_params(void *data, const struct voidfx_frame *frame)
{
	struct chromatic_aberration *ca = data;

	/* Jitter wobbles the split between 0 and 2x the configured strength. */
	float wobble = fx_value_noise(frame->time * ca->jitter_speed) * 2.0f - 1.0f;
	float strength = ca->strength * (1.0f + ca->jitter * wobble);

	struct vec2 uv_size;
	vec2_set(&uv_size, (float)frame->width, (float)frame->height);

	gs_effect_set_vec2(ca->p_uv_size, &uv_size);
	gs_effect_set_float(ca->p_time, frame->time);
	gs_effect_set_float(ca->p_radial, ca->mode == CA_MODE_RADIAL ? 1.0f : 0.0f);
	gs_effect_set_float(ca->p_strength, strength);
	gs_effect_set_vec2(ca->p_direction, &ca->direction);
	gs_effect_set_vec2(ca->p_center, &ca->center);
	gs_effect_set_float(ca->p_falloff, ca->falloff);
	gs_effect_set_float(ca->p_scanline_intensity, ca->scanlines);
	gs_effect_set_float(ca->p_scanline_spacing, ca->scanline_spacing);
	gs_effect_set_float(ca->p_noise_amount, ca->noise);
	gs_effect_set_float(ca->p_tracking_amount, ca->tracking);
	gs_effect_set_float(ca->p_bleed, ca->bleed);
	gs_effect_set_float(ca->p_washed_out, ca->washed_out);
}

const struct voidfx_effect_info voidfx_chromatic_aberration = {
	.id = "chromatic_aberration",
	.name_key = "Effect.ChromaticAberration",
	.group_name = "ca_group",
	.effect_file = "effects/chromatic-aberration.effect",
	.create = ca_create,
	.destroy = ca_destroy,
	.get_defaults = ca_get_defaults,
	.add_properties = ca_add_properties,
	.update = ca_update,
	.set_params = ca_set_params,
};
