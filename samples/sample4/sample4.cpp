/* OpenSceneGraph example, osgterrain.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include "ov_BillboardLayer.h"
#include "ov_BillboardLayerStateSet.h"
#include "ov_TerrainSplatShadingStateSet.h"
#include "ov_Utils.h"
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
#include <osg/PositionAttitudeTransform>
#include <osg/Fog>
#include <osg/ShapeDrawable>
#include <iostream>
#include "ov_DemoTerrain.h"
#include "ov_DemoShadow.h"

std::vector<osgVegetation::BillboardLayer> GetVegetationLayers()
{
	std::vector<osgVegetation::BillboardLayer> layers;
	osgVegetation::BillboardLayer grass_data(100, 0.1, 1.0, 0.1, 5);
	grass_data.Type = osgVegetation::BillboardLayer::BLT_GRASS;
	grass_data.Filter.SplatFilter = osgVegetation::PassFilter::GenerateSplatFilter(osg::Vec4(-1, 0.5, -1, -1), "<");
	//grass_data.Type = osgVegetation::BillboardLayer::BLT_CROSS_QUADS;
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/veg_plant03.png", osg::Vec2f(4, 2), 0.9, 0.008));
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/veg_plant01.png", osg::Vec2f(2, 2), 0.9, 0.002));
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/grass2.png", osg::Vec2f(2, 1), 1.0, 1.0));
	layers.push_back(grass_data);

	osgVegetation::BillboardLayer grass_data2(30, 0.4, 1.0, 0.1, 5);
	grass_data2.Type = osgVegetation::BillboardLayer::BLT_GRASS;
	grass_data2.Filter.SplatFilter = grass_data.Filter.SplatFilter;
	grass_data2.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/grass2.png", osg::Vec2f(2, 1), 1.0, 1.0));
	layers.push_back(grass_data2);

	//osgVegetation::BillboardLayer tree_data(2740, 0.001, 0.5, 0.7, 0.1, 2);

	osgVegetation::BillboardLayer tree_data(740, 0.01, 0.7, 0.1, 2);
	tree_data.Filter.SplatFilter = osgVegetation::PassFilter::GenerateSplatFilter(osg::Vec4(-1, 0.5, -1, -1), "<");
	tree_data.Filter.ColorFilter = "if(length(base_color.xyz) > 0.5) return false;";
	//tree_data.Type = osgVegetation::BillboardLayer::BLT_CROSS_QUADS;
	tree_data.Type = osgVegetation::BillboardLayer::BLT_ROTATED_QUAD;
	tree_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/fir01_bb.png", osg::Vec2f(10, 16), 1.5, 1.0));
	layers.push_back(tree_data);
	return layers;
}

osgVegetation::TerrainSplatShadingConfig GetTerrainShaderConfig(bool tess)
{
	//Create terrain layer node
	osgVegetation::TerrainSplatShadingConfig tsc;
	tsc.ColorTexture = osgVegetation::TextureConfig("Images/lz.rgb", 0);
	tsc.SplatTexture = osgVegetation::TextureConfig("Images/lz_coverage.png", 1);
	tsc.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.08));
	tsc.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 0.08));
	tsc.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 0.08));
	tsc.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 0.08));
	tsc.DetailTextureUnit = 3;

	tsc.NoiseTexture = osgVegetation::TextureConfig("terrain/detail/noise.png", 2);
	tsc.UseTessellation = tess;
	return tsc;
}

osg::ref_ptr<osg::Node> CreateVegetationNode(osg::ref_ptr<osg::Node> terrain_geometry)
{
	osg::ref_ptr<osg::Group> veg_group = new osg::Group();
	int billboard_tex_unit = 5;
	for (size_t i = 0; i < GetVegetationLayers().size(); i++)
	{
		osgVegetation::BillboardLayer layer_config = GetVegetationLayers().at(i);
		osg::ref_ptr<osg::Group> bb_layer = new osgVegetation::BillboardLayerEffect(layer_config, billboard_tex_unit);
		bb_layer->addChild(terrain_geometry);
		if (layer_config.Type == osgVegetation::BillboardLayer::BLT_GRASS)
			bb_layer->setNodeMask(ReceivesShadowTraversalMask);
		else
			bb_layer->setNodeMask(ReceivesShadowTraversalMask | CastsShadowTraversalMask);
		veg_group->addChild(bb_layer);
	}
	return veg_group;
}

osg::ref_ptr<osg::Group> CreateTerrainPatches(double terrain_size)
{
	//Create terrain geometry used for both terrain layer and  vegetation layers
	osg::ref_ptr<osg::Node> terrain_geometry = CreateDemoTerrain(terrain_size);
	osgVegetation::ConvertToPatches(terrain_geometry);
	osg::ref_ptr<osgVegetation::TerrainShadingEffect> terrain_shading_effect = new osgVegetation::TerrainShadingEffect(GetTerrainShaderConfig(true));
	//Disable terrain self shadowning
	//terrain_shading_effect->setNodeMask(ReceivesShadowTraversalMask);
	terrain_shading_effect->addChild(terrain_geometry);
	//Create vegetation layer node
	terrain_shading_effect->addChild( CreateVegetationNode(terrain_geometry));
	return terrain_shading_effect;
}

osg::ref_ptr<osg::Group> CreateTerrain(double terrain_size)
{
	//Create terrain geometry used for both terrain layer and  vegetation layers
	osg::ref_ptr<osg::Node> terrain_geometry = CreateDemoTerrain(terrain_size);

	const bool apply_shader = true;
	osg::ref_ptr<osg::Group> terrain_shading_effect = apply_shader ? new osgVegetation::TerrainSplatShadingEffect(GetTerrainShaderConfig(false)) : new osg::Group();
	//Disable terrain self shadowning
	//terrain_shading_effect->setNodeMask(ReceivesShadowTraversalMask);
	terrain_shading_effect->addChild(terrain_geometry);

	//Create vegetation layer node
	//minimal copy for our sample data
	osg::ref_ptr<osg::Node> terrain_patch_geometry = dynamic_cast<osg::Node*>(terrain_geometry->clone(osg::CopyOp::DEEP_COPY_PRIMITIVES | osg::CopyOp::DEEP_COPY_DRAWABLES));
	osgVegetation::ConvertToPatches(terrain_patch_geometry);
	terrain_shading_effect->addChild( CreateVegetationNode(terrain_patch_geometry));
	return terrain_shading_effect;
}

int main(int argc, char** argv)
{
	osgVegetation::SceneConfiguration config;
	config.ShadowMode = osgVegetation::SM_VDSM1;
	config.FogMode = osgVegetation::FM_EXP2;

	osg::ArgumentParser arguments(&argc, argv);

	//osg::DisplaySettings::instance()->setShaderHint(osg::DisplaySettings::SHADER_GL3);

	// construct the viewer.
	osgViewer::Viewer viewer(arguments);

	// set up the camera manipulators.
	{
		osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;
		keyswitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator());
		keyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator());
		keyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator());
		keyswitchManipulator->addMatrixManipulator('4', "Terrain", new osgGA::TerrainManipulator());
		viewer.setCameraManipulator(keyswitchManipulator.get());
	}

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the record camera path handler
	viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	osg::DisplaySettings::instance()->setNumMultiSamples(4);

	//Add sample data path
	osgDB::Registry::instance()->getDataFilePathList().push_back("../data");

	osg::ref_ptr<osg::Group> root_node = CreateShadowNode(config.ShadowMode);

	const double terrain_size = 2000;
	const bool terrain_patches = false;
	osg::ref_ptr<osg::Group> terrain_and_vegetation_node = terrain_patches ? CreateTerrainPatches(terrain_size) : CreateTerrain(terrain_size);

	//apply scene settings to terrain and vegetation shaders
	osgVegetation::SetSceneDefinitions(terrain_and_vegetation_node->getOrCreateStateSet(), config);

	root_node->addChild(terrain_and_vegetation_node);

	if (!root_node)
	{
		osg::notify(osg::NOTICE) << "Warning: no valid data loaded, please specify a database on the command line." << std::endl;
		return 1;
	}

	//Add light and shadows
	osg::Light* light = new osg::Light;
	light->setDiffuse(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	osg::Vec4 light_pos(1, 1.0, 1, 0);
	light->setPosition(light_pos);		// last param	w = 0.0 directional light (direction)
	osg::Vec3f light_dir(-light_pos.x(), -light_pos.y(), -light_pos.z());
	light_dir.normalize();
	light->setDirection(light_dir);
	light->setAmbient(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));

	osg::LightSource* light_source = new osg::LightSource;
	light_source->setLight(light);
	root_node->addChild(light_source);

	viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

	if (config.FogMode != osgVegetation::FM_DISABLED) //Add fog effect?
	{
		const osg::Vec4 fog_color(0.5, 0.6, 0.7, 1.0);
		osg::StateSet* state = root_node->getOrCreateStateSet();
		osg::ref_ptr<osg::Fog> fog = new osg::Fog();
		state->setMode(GL_FOG, osg::StateAttribute::ON);
		state->setAttributeAndModes(fog.get());
		fog->setMode(osg::Fog::Mode(config.FogMode));
		fog->setDensity(0.0005);
		fog->setColor(fog_color);
		if (config.FogMode == osgVegetation::FM_LINEAR)
		{
			fog->setStart(0);
			fog->setEnd(1000);
		}

		viewer.getCamera()->setClearColor(fog_color);
	}

	viewer.setSceneData(root_node);
	viewer.setUpViewInWindow(100, 100, 800, 600);

	viewer.realize();
	viewer.getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
	//viewer.getCamera()->getGraphicsContext()->getState()->setUseVertexAttributeAliasing(true);

	// run the viewers main loop
	while (!viewer.done())
	{
		float t = viewer.getFrameStamp()->getSimulationTime() * 0.5;
		light_pos.set(sinf(t), cosf(t), 1, 0.0f);
		light->setPosition(light_pos);
		light_dir.set(-light_pos.x(), -light_pos.y(), -light_pos.z());
		light_dir.normalize();
		light->setDirection(light_dir);

		viewer.frame();
	}
	return 0;
}
