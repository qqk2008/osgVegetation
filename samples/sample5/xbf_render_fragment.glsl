#version 330 compatibility
#pragma import_defines (OV_BILLBOARD)
#extension GL_ARB_gpu_shader5 : enable 
uniform sampler2D ov_color_texture;
uniform float ov_FadeDistance;
uniform float ov_StartDistance;
uniform float ov_EndDistance;
in vec2 ov_tex_coord0;
in vec3 ov_normal;
in float ov_depth;
in float ov_intensity;
in mat3 ov_normal_matrix;
void main(void) { 
	vec4 base_color = texture2D(ov_color_texture, ov_tex_coord0.xy);
	base_color.xyz *= ov_intensity;
	vec3 light_dir = normalize(gl_LightSource[0].position.xyz);

#ifdef OV_BILLBOARD
	vec4 coded_normal = texture2D(ov_color_texture, ov_tex_coord0.xy - vec2(0.0,0.5));
	vec3 normal = ov_normal_matrix * normalize(coded_normal.xyz * 2.0 - 1);
	vec3 quad_normal = ov_normal;
	quad_normal.y = 0;
	quad_normal = normalize(quad_normal);
	float NdotC = max(dot(quad_normal, vec3(0,0,1)), 0);
	//if(NdotC < 0.9) NdotC = 0.0; else NdotC = 1.0;
	base_color.a = 1.3 * mix(base_color.a, 0.0, 1 - NdotC);
	base_color.a = mix(0, base_color.a, clamp((ov_depth - (ov_StartDistance - ov_FadeDistance/2.0))/ov_FadeDistance, 0, 1));
	base_color.xyz *= 1.4;
#else
	vec3 normal = normalize(ov_normal);
	base_color.a = mix(base_color.a, 0, clamp((ov_depth - (ov_EndDistance + ov_FadeDistance*0.5)) / ov_FadeDistance, 0, 1));
#endif
	float NdotL = max(dot(normal, light_dir), 0);
	base_color.xyz *= (NdotL * gl_LightSource[0].diffuse.xyz * gl_FrontMaterial.diffuse.xyz + gl_LightSource[0].ambient.xyz * gl_FrontMaterial.ambient.xyz);
	gl_FragColor = base_color;
}