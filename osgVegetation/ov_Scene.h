#pragma once
#include "ov_Common.h"
#include <osg/Fog>
#include <sstream>

namespace osgVegetation
{
	class Scene
	{
	public:
		static void SetLighting(osg::StateSet* state_set, bool value)
		{
			state_set->setDefine("OSG_LIGHTING", value ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
		}

		static void DisableFog(osg::StateSet* state_set)
		{
			state_set->removeDefine("OSG_FOG_MODE");
		}

		static void EnableFog(osg::StateSet* state_set, osg::Fog::Mode fog_mode)
		{
			const std::string fog_mode_str = "OSG_FOG_MODE";
			switch (fog_mode)
			{
			case osg::Fog::Mode::LINEAR:
				state_set->setDefine(fog_mode_str, "1");
				break;
			case osg::Fog::Mode::EXP:
				state_set->setDefine(fog_mode_str, "2");
				break;
			case osg::Fog::Mode::EXP2:
				state_set->setDefine(fog_mode_str, "3");
				break;
			}
		}

		static void EnableShadowMapping(osg::StateSet* state_set, unsigned int num_shadow_maps)
		{
			const std::string shadow_mode_str = "OSG_NUM_SHADOW_MAPS";
			std::stringstream ss;
			ss << num_shadow_maps;
			state_set->setDefine(shadow_mode_str, ss.str());
		}
	};
}