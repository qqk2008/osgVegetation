#pragma once
#include "ov_Common.h"
#include "ov_TerrainStateSet.h"
#include <osg/Vec2>
#include <osg/Program>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <vector>

namespace osgVegetation
{

	/*class DetailLayer
	{
	public:
		DetailLayer(const std::string &texture, float scale = 1) : DetailTexture(texture), Scale(scale) {}
		std::string DetailTexture;
		float Scale;
	};

	class SplatMappingConfig
	{
	public:
		SplatMappingConfig() : MaxDistance(1000) , DetailTextureUnit(-1) {}
		std::vector<DetailLayer> DetailLayers;
		int DetailTextureUnit;
		TextureConfig SplatTexture;
		float MaxDistance;
	};*/

	class TerrainShadingStateSetConfig : public TerrainStateSetConfig
	{
	public:
		TerrainShadingStateSetConfig() : UseTessellation(false)
		{
			
		}
		//SplatMappingConfig SplatMapping;
		TextureConfig NoiseTexture;
		bool UseTessellation;
	};

	class TerrainShadingStateSet : public TerrainStateSet
	{
	public:
		TerrainShadingStateSet(const TerrainShadingStateSetConfig& config) : TerrainStateSet(config)
		{
			if (config.UseTessellation)
				setDefine("OV_TERRAIN_TESSELLATION");

			_SetNoiseTexture(config.NoiseTexture);
			//_SetupSplatMapping(config.SplatMapping);

			osg::Program* program = _CreateProgram(config);
			setAttribute(program, osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
		}

	private:
		osg::Program* _CreateProgram(const TerrainShadingStateSetConfig& config)
		{
			osg::Program* program = new osg::Program;
			program->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_terrain_vertex.glsl")));
			program->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_terrain_elevation.glsl")));
			program->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_common_vertex.glsl")));
			if (config.UseTessellation)
			{
				program->addShader(osg::Shader::readShaderFile(osg::Shader::TESSCONTROL, osgDB::findDataFile("ov_terrain_tess_ctrl.glsl")));
				program->addShader(osg::Shader::readShaderFile(osg::Shader::TESSEVALUATION, osgDB::findDataFile("ov_common_vertex.glsl")));
				program->addShader(osg::Shader::readShaderFile(osg::Shader::TESSEVALUATION, osgDB::findDataFile("ov_terrain_elevation.glsl")));
				program->addShader(osg::Shader::readShaderFile(osg::Shader::TESSEVALUATION, osgDB::findDataFile("ov_terrain_tess_eval.glsl")));
			}
			program->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_common_fragment.glsl")));
			program->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_terrain_color.glsl")));
			program->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_terrain_fragment.glsl")));
			return program;
		}

		void _SetNoiseTexture(TextureConfig config)
		{
			_ApplyTextureConfig(config);
			if (config.TexUnit >= 0)
			{
				addUniform(new osg::Uniform("ov_noise_texture", config.TexUnit));
				setDefine("OV_NOISE_TEXTURE");
			}
		}
	};

	class TerrainShadingEffect : public osg::Group
	{
	public:
		TerrainShadingEffect(const TerrainShadingStateSetConfig& config)
		{
			osg::ref_ptr<osg::StateSet> state_set = new TerrainShadingStateSet(config);
			setStateSet(state_set);
		}
	};
}