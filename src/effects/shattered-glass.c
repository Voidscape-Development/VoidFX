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
#include <graphics/vec4.h>
#include <util/bmem.h>

#include "../voidfx-effect.h"
#include "fx-util.h"

#define S_PATTERN "sg_pattern"
#define S_SHARD_SIZE "sg_shard_size"
#define S_RADIAL_CRACKS "sg_radial_cracks"
#define S_RINGS "sg_rings"
#define S_IMPACT_X "sg_impact_x"
#define S_IMPACT_Y "sg_impact_y"
#define S_IRREGULARITY "sg_irregularity"
#define S_SEED "sg_seed"
#define S_SHATTER "sg_shatter"
#define S_DISPLACEMENT "sg_displacement"
#define S_ROTATION "sg_rotation"
#define S_SCATTER "sg_scatter"
#define S_CRACK_WIDTH "sg_crack_width"
#define S_CRACK_COLOR "sg_crack_color"
#define S_GAP_WIDTH "sg_gap_width"
#define S_EDGE_HIGHLIGHT "sg_edge_highlight"
#define S_SHADING "sg_shading"

enum sg_pattern {
	SG_PATTERN_RANDOM = 0,
	SG_PATTERN_IMPACT = 1,
};

struct shattered_glass {
	gs_effect_t *effect;

	gs_eparam_t *p_uv_size;
	gs_eparam_t *p_impact_pattern;
	gs_eparam_t *p_shard_size;
	gs_eparam_t *p_radial_count;
	gs_eparam_t *p_ring_count;
	gs_eparam_t *p_impact;
	gs_eparam_t *p_irregularity;
	gs_eparam_t *p_seed;
	gs_eparam_t *p_displacement;
	gs_eparam_t *p_rotation;
	gs_eparam_t *p_scatter;
	gs_eparam_t *p_crack_width;
	gs_eparam_t *p_crack_color;
	gs_eparam_t *p_gap_width;
	gs_eparam_t *p_edge_highlight;
	gs_eparam_t *p_shading;

	enum sg_pattern pattern;
	float shard_size;
	float radial_count;
	float ring_count;
	struct vec2 impact;
	float irregularity;
	struct vec2 seed;
	float displacement;
	float rotation;
	float scatter;
	float crack_width;
	struct vec4 crack_color;
	float gap_width;
	float edge_highlight;
	float shading;
};

static void *sg_create(gs_effect_t *effect)
{
	struct shattered_glass *sg = bzalloc(sizeof(*sg));
	sg->effect = effect;

	sg->p_uv_size = gs_effect_get_param_by_name(effect, "uv_size");
	sg->p_impact_pattern = gs_effect_get_param_by_name(effect, "impact_pattern");
	sg->p_shard_size = gs_effect_get_param_by_name(effect, "shard_size");
	sg->p_radial_count = gs_effect_get_param_by_name(effect, "radial_count");
	sg->p_ring_count = gs_effect_get_param_by_name(effect, "ring_count");
	sg->p_impact = gs_effect_get_param_by_name(effect, "impact");
	sg->p_irregularity = gs_effect_get_param_by_name(effect, "irregularity");
	sg->p_seed = gs_effect_get_param_by_name(effect, "seed");
	sg->p_displacement = gs_effect_get_param_by_name(effect, "displacement");
	sg->p_rotation = gs_effect_get_param_by_name(effect, "rotation");
	sg->p_scatter = gs_effect_get_param_by_name(effect, "scatter");
	sg->p_crack_width = gs_effect_get_param_by_name(effect, "crack_width");
	sg->p_crack_color = gs_effect_get_param_by_name(effect, "crack_color");
	sg->p_gap_width = gs_effect_get_param_by_name(effect, "gap_width");
	sg->p_edge_highlight = gs_effect_get_param_by_name(effect, "edge_highlight");
	sg->p_shading = gs_effect_get_param_by_name(effect, "shading");

	return sg;
}

static void sg_destroy(void *data)
{
	bfree(data);
}

static void sg_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_PATTERN, SG_PATTERN_IMPACT);
	obs_data_set_default_double(settings, S_SHARD_SIZE, 140.0);
	obs_data_set_default_int(settings, S_RADIAL_CRACKS, 14);
	obs_data_set_default_int(settings, S_RINGS, 5);
	obs_data_set_default_double(settings, S_IMPACT_X, 50.0);
	obs_data_set_default_double(settings, S_IMPACT_Y, 50.0);
	obs_data_set_default_double(settings, S_IRREGULARITY, 85.0);
	obs_data_set_default_int(settings, S_SEED, 1);
	obs_data_set_default_double(settings, S_SHATTER, 100.0);
	obs_data_set_default_double(settings, S_DISPLACEMENT, 8.0);
	obs_data_set_default_double(settings, S_ROTATION, 2.0);
	obs_data_set_default_double(settings, S_SCATTER, 60.0);
	obs_data_set_default_double(settings, S_CRACK_WIDTH, 1.5);
	obs_data_set_default_int(settings, S_CRACK_COLOR, 0xE6FFFFFF);
	obs_data_set_default_double(settings, S_GAP_WIDTH, 0.0);
	obs_data_set_default_double(settings, S_EDGE_HIGHLIGHT, 0.35);
	obs_data_set_default_double(settings, S_SHADING, 0.12);
}

static bool sg_pattern_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);

	bool impact = obs_data_get_int(settings, S_PATTERN) == SG_PATTERN_IMPACT;
	obs_property_set_visible(obs_properties_get(props, S_SHARD_SIZE), !impact);
	obs_property_set_visible(obs_properties_get(props, S_RADIAL_CRACKS), impact);
	obs_property_set_visible(obs_properties_get(props, S_RINGS), impact);
	return true;
}

static void sg_add_properties(obs_properties_t *group)
{
	obs_property_t *p;

	p = obs_properties_add_list(group, S_PATTERN, obs_module_text("SG.Pattern"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("SG.Pattern.Impact"), SG_PATTERN_IMPACT);
	obs_property_list_add_int(p, obs_module_text("SG.Pattern.Random"), SG_PATTERN_RANDOM);
	obs_property_set_modified_callback(p, sg_pattern_modified);

	p = obs_properties_add_float_slider(group, S_SHARD_SIZE, obs_module_text("SG.ShardSize"), 16.0, 800.0, 1.0);
	obs_property_float_set_suffix(p, " px");
	obs_properties_add_int_slider(group, S_RADIAL_CRACKS, obs_module_text("SG.RadialCracks"), 3, 64, 1);
	obs_properties_add_int_slider(group, S_RINGS, obs_module_text("SG.Rings"), 1, 30, 1);

	p = obs_properties_add_float_slider(group, S_IMPACT_X, obs_module_text("SG.ImpactX"), 0.0, 100.0, 0.5);
	obs_property_float_set_suffix(p, "%");
	p = obs_properties_add_float_slider(group, S_IMPACT_Y, obs_module_text("SG.ImpactY"), 0.0, 100.0, 0.5);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(group, S_IRREGULARITY, obs_module_text("SG.Irregularity"), 0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "%");
	obs_properties_add_int(group, S_SEED, obs_module_text("SG.Seed"), 0, 9999, 1);

	p = obs_properties_add_float_slider(group, S_SHATTER, obs_module_text("SG.Shatter"), 0.0, 100.0, 0.5);
	obs_property_float_set_suffix(p, "%");
	obs_property_set_long_description(p, obs_module_text("SG.Shatter.Description"));
	p = obs_properties_add_float_slider(group, S_DISPLACEMENT, obs_module_text("SG.Displacement"), 0.0, 200.0, 0.5);
	obs_property_float_set_suffix(p, " px");
	p = obs_properties_add_float_slider(group, S_ROTATION, obs_module_text("SG.Rotation"), 0.0, 45.0, 0.1);
	obs_property_float_set_suffix(p, "°");
	p = obs_properties_add_float_slider(group, S_SCATTER, obs_module_text("SG.Scatter"), 0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(group, S_CRACK_WIDTH, obs_module_text("SG.CrackWidth"), 0.0, 20.0, 0.1);
	obs_property_float_set_suffix(p, " px");
	obs_properties_add_color_alpha(group, S_CRACK_COLOR, obs_module_text("SG.CrackColor"));
	p = obs_properties_add_float_slider(group, S_GAP_WIDTH, obs_module_text("SG.GapWidth"), 0.0, 40.0, 0.1);
	obs_property_float_set_suffix(p, " px");
	obs_properties_add_float_slider(group, S_EDGE_HIGHLIGHT, obs_module_text("SG.EdgeHighlight"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(group, S_SHADING, obs_module_text("SG.Shading"), 0.0, 0.5, 0.01);
}

static void sg_update(void *data, obs_data_t *settings)
{
	struct shattered_glass *sg = data;

	sg->pattern = (enum sg_pattern)obs_data_get_int(settings, S_PATTERN);
	sg->shard_size = (float)obs_data_get_double(settings, S_SHARD_SIZE);
	sg->radial_count = (float)obs_data_get_int(settings, S_RADIAL_CRACKS);
	sg->ring_count = (float)obs_data_get_int(settings, S_RINGS);
	vec2_set(&sg->impact, (float)obs_data_get_double(settings, S_IMPACT_X) / 100.0f,
		 (float)obs_data_get_double(settings, S_IMPACT_Y) / 100.0f);
	sg->irregularity = (float)obs_data_get_double(settings, S_IRREGULARITY) / 100.0f;

	/* Map the integer seed to a pattern offset; keep it small for GPU precision. */
	int32_t seed = (int32_t)obs_data_get_int(settings, S_SEED);
	vec2_set(&sg->seed, fx_hash(seed * 2) * 512.0f, fx_hash(seed * 2 + 1) * 512.0f);

	float shatter = (float)obs_data_get_double(settings, S_SHATTER) / 100.0f;
	sg->displacement = (float)obs_data_get_double(settings, S_DISPLACEMENT) * shatter;
	sg->rotation = RAD((float)obs_data_get_double(settings, S_ROTATION)) * shatter;
	sg->scatter = (float)obs_data_get_double(settings, S_SCATTER) / 100.0f;

	sg->crack_width = (float)obs_data_get_double(settings, S_CRACK_WIDTH);
	vec4_from_rgba(&sg->crack_color, (uint32_t)obs_data_get_int(settings, S_CRACK_COLOR));
	sg->gap_width = (float)obs_data_get_double(settings, S_GAP_WIDTH);
	sg->edge_highlight = (float)obs_data_get_double(settings, S_EDGE_HIGHLIGHT);
	sg->shading = (float)obs_data_get_double(settings, S_SHADING);
}

static void sg_set_params(void *data, const struct voidfx_frame *frame)
{
	struct shattered_glass *sg = data;

	struct vec2 uv_size;
	vec2_set(&uv_size, (float)frame->width, (float)frame->height);

	gs_effect_set_vec2(sg->p_uv_size, &uv_size);
	gs_effect_set_float(sg->p_impact_pattern, sg->pattern == SG_PATTERN_IMPACT ? 1.0f : 0.0f);
	gs_effect_set_float(sg->p_shard_size, sg->shard_size);
	gs_effect_set_float(sg->p_radial_count, sg->radial_count);
	gs_effect_set_float(sg->p_ring_count, sg->ring_count);
	gs_effect_set_vec2(sg->p_impact, &sg->impact);
	gs_effect_set_float(sg->p_irregularity, sg->irregularity);
	gs_effect_set_vec2(sg->p_seed, &sg->seed);
	gs_effect_set_float(sg->p_displacement, sg->displacement);
	gs_effect_set_float(sg->p_rotation, sg->rotation);
	gs_effect_set_float(sg->p_scatter, sg->scatter);
	gs_effect_set_float(sg->p_crack_width, sg->crack_width);
	gs_effect_set_vec4(sg->p_crack_color, &sg->crack_color);
	gs_effect_set_float(sg->p_gap_width, sg->gap_width);
	gs_effect_set_float(sg->p_edge_highlight, sg->edge_highlight);
	gs_effect_set_float(sg->p_shading, sg->shading);
}

const struct voidfx_effect_info voidfx_shattered_glass = {
	.id = "shattered_glass",
	.name_key = "Effect.ShatteredGlass",
	.group_name = "sg_group",
	.effect_file = "effects/shattered-glass.effect",
	.create = sg_create,
	.destroy = sg_destroy,
	.get_defaults = sg_get_defaults,
	.add_properties = sg_add_properties,
	.update = sg_update,
	.set_params = sg_set_params,
};
