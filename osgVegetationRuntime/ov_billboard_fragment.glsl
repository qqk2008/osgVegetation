#version 120
#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_texture_array : enable
#pragma import_defines (FM_LINEAR,FM_EXP,FM_EXP2)
uniform sampler2DArray ov_billboard_texture;
varying vec2 ov_geometry_texcoord;
varying vec4 ov_geometry_color;
flat varying int ov_geometry_tex_index;

void main(void) 
{
	vec4 out_color = ov_geometry_color*texture2DArray(ov_billboard_texture, vec3(ov_geometry_texcoord, ov_geometry_tex_index));
#if defined(FM_LINEAR) || defined(FM_EXP) || defined(FM_EXP2)
	float depth = gl_FragCoord.z / gl_FragCoord.w;

#ifdef FM_LINEAR
	float fog_factor = (gl_Fog.end - depth) * gl_Fog.scale;
#endif

#ifdef FM_EXP
	float fog_factor = exp(-gl_Fog.density * depth);
#endif

#ifdef FM_EXP2
	float fog_factor = exp(-pow((gl_Fog.density * depth), 2.0));
#endif
	fog_factor = clamp(fog_factor, 0.0, 1.0);
	out_color.xyz = mix(gl_Fog.color.xyz, out_color.xyz, fog_factor);
#endif
	gl_FragColor = out_color;
}