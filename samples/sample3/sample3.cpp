#include "ov_VPBVegetationInjection.h"
#include "ov_Utils.h"
#include "ov_TerrainSplatShadingStateSet.h"
#include <osg/ArgumentParser>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osg/Version>
#include <osg/Fog>
#include "ov_Demo.h"
#include <iostream>

int main(int argc, char** argv)
{
	
	osgVegetation::Register.Scene.Shadow.Mode = osgVegetation::SM_LISPSM;
	osgVegetation::Register.Scene.FogMode = osgVegetation::FM_EXP2;
	//osgVegetation::Register.TexUnits.AddUnit(12, OV_BILLBOARD_TEXTURE_ID);
	osgVegetation::Register.TexUnits.AddUnit(0, OV_TERRAIN_COLOR_TEXTURE_ID);
	osgVegetation::Register.TexUnits.AddUnit(1, OV_TERRAIN_SPLAT_TEXTURE_ID);

	Demo demo(argc, argv, osgVegetation::Register.Scene);

	//setup vegetation layers
	std::vector<osgVegetation::BillboardLayerConfig> layers;
	osgVegetation::BillboardLayerConfig grass_layer;
	grass_layer.MaxDistance = 140;
	grass_layer.Density = 0.2;
	grass_layer.ColorImpact = 1.0;
	grass_layer.CastShadow = false;
	grass_layer.Type = osgVegetation::BillboardLayerConfig::BLT_GRASS;
	grass_layer.Billboards.push_back(osgVegetation::BillboardLayerConfig::Billboard("billboards/veg_plant03.png", osg::Vec2f(4, 2), 0.9, 0.008));
	grass_layer.Billboards.push_back(osgVegetation::BillboardLayerConfig::Billboard("billboards/veg_plant01.png", osg::Vec2f(2, 2), 0.9, 0.002));
	grass_layer.Billboards.push_back(osgVegetation::BillboardLayerConfig::Billboard("billboards/grass2.png", osg::Vec2f(2, 1), 1.0, 1.0));
	//osg::Vec4 grass_splat_color_filter(0.1, -1, -1, -1);
	grass_layer.Filter.SplatFilter = "if(splat_color.x > 0.5) return false;"; //osgVegetation::PassFilter::GenerateSplatFilter(grass_splat_color_filter, "<");

	osgVegetation::BillboardLayerConfig tree_layer;
	tree_layer.MaxDistance = 740;
	tree_layer.Density = 0.001;
	tree_layer.ColorImpact = 0.0;
	tree_layer.Type = osgVegetation::BillboardLayerConfig::BLT_ROTATED_QUAD;
	tree_layer.Filter.ColorFilter = "if(length(base_color.xyz) > 0.5) return false;";
	tree_layer.Filter.SplatFilter = "if(splat_color.x > 0.5) return false;";
	tree_layer.Billboards.push_back(osgVegetation::BillboardLayerConfig::Billboard("billboards/fir01_bb.png", osg::Vec2f(10, 16), 1.0, 1.0));
	//tree_data.Billboards.push_back(osgVegetation::BillboardLayerConfig::Billboard("billboards/tree0.rgba", osg::Vec2f(8, 16), 1.2));

	//Setup layers in terrain injection lods
	std::vector<osgVegetation::VPBInjectionLODConfig> terrain_lods;
	osgVegetation::VPBInjectionLODConfig grass_terrain_lod(5);
	grass_terrain_lod.Layers.push_back(&grass_layer);
	terrain_lods.push_back(grass_terrain_lod);

	osgVegetation::VPBInjectionLODConfig tree_terrain_lod(2);
	tree_terrain_lod.Layers.push_back(&tree_layer);
	terrain_lods.push_back(tree_terrain_lod);

	osgDB::Registry::instance()->setReadFileCallback(new osgVegetation::VPBVegetationInjection(terrain_lods));
#if 1
	osg::ref_ptr<osg::Node> terrain_node = osgDB::readNodeFile("terrain/us-terrain.zip/us-terrain.osg");
#else
	//osg::ref_ptr<osg::Node> terrain_node = osgDB::readNodeFile("D:/terrain/vpb/us/final/us-terrain.osgb");
	osg::ref_ptr<osg::Node> terrain_node = osgDB::readNodeFile("D:/terrain/vpb/us/final/us-terrain.osg");
#endif

	osgVegetation::TerrainSplatShadingConfig terrain_shading;
	//terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 0.2));
	//terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.2));
	//terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 0.2));
	//terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.2));
	terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 0.05));
	terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.05));
	terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.05));
	terrain_shading.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.05));
	terrain_shading.NoiseTexture.File = "terrain/detail/noise.png";
	terrain_shading.ColorModulateRatio = 0.5;
	osg::ref_ptr <osgVegetation::TerrainSplatShadingStateSet> terrain_shading_ss = new osgVegetation::TerrainSplatShadingStateSet(terrain_shading);
	osg::ref_ptr<osg::Group> terrain_shading_effect = new osg::Group();
	terrain_shading_effect->setStateSet(terrain_shading_ss);
	terrain_shading_effect->addChild(terrain_node);

	demo.GetSceneRoot()->addChild(terrain_shading_effect);
	demo.Run();
	return 0;
}
