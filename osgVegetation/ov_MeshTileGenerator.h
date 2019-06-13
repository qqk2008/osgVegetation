#pragma once
#include "ov_GPUCullData.h"
#include "ov_MeshTileGeneratorConfig.h"
#include <osg/CullFace>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

namespace osgVegetation
{

	//Hack to avoid gl-crash on first update
	static bool _firstTerrainDrawDone = false;

	class MeshLayerGenerator
	{
	private:

		// We must ensure that cull shader finished filling indirect commands and indirect targets, before draw shader
		// starts using them. We use glMemoryBarrier() barrier to achieve that.
		// It is also possible that we should use glMemoryBarrier() after resetting textures, but i implemented that only for
		// dynamic rendering.
		struct TerrainGeometryDrawCB : public osg::Drawable::DrawCallback
		{
			TerrainGeometryDrawCB(GLbitfield barriers)
				: _barriers(barriers)
			{
			}

			virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
			{
				//std::cout << "Terrain Draw\n";
				renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(_barriers);
				drawable->drawImplementation(renderInfo);
				_firstTerrainDrawDone = true;
			}
			GLbitfield _barriers;
		};

		struct InstanceGeometryDrawCB : public osg::Drawable::DrawCallback
		{
			InstanceGeometryDrawCB(osg::TextureBuffer* tex, osg::BindImageTexture* binding) : _texture(tex), _binding(binding)
			{

			}

			virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
			{
				//osg::BufferData* img = const_cast<osg::BufferData*>(_texture->getBufferData());
				//if (img != NULL)
				//{
					//img->dirty();
					//_binding->apply(*renderInfo.getState());
					//_texture->dirtyTextureParameters();
					//apply(*renderInfo.getState());
				//}
				//_texture->dirtyTextureObject();
				//_texture->dirtyTextureParameters();
				//_texture->apply(*renderInfo.getState());
				if (_firstTerrainDrawDone)
				{
					//std::cout << "Geom Draw\n";

					renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
					drawable->drawImplementation(renderInfo);

					if (osg::BufferData* img = const_cast<osg::BufferData*>(_texture->getBufferData()))
						img->dirty();
				}
			}
			osg::TextureBuffer* _texture;
			osg::BindImageTexture* _binding;
		};

		class DrawCallbackVisitor : public osg::NodeVisitor
		{
		public:
			DrawCallbackVisitor() :
				osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {
			}

			void apply(osg::Node& node)
			{
				osg::Geometry* geom = dynamic_cast<osg::Geometry*>(&node);
				if (geom)
				{
					if (!geom->getDrawCallback())
						geom->setDrawCallback(new TerrainGeometryDrawCB(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
				}
				else
				{
					traverse(node);
				}
			}
		};


		struct BoundingBoxCB : public osg::Drawable::ComputeBoundingBoxCallback
		{
			BoundingBoxCB() {}
			BoundingBoxCB(const osg::BoundingBox &bbox) : _bbox(bbox) {};
			osg::BoundingBox computeBound(const osg::Drawable&) const { return _bbox; }
		private:
			osg::BoundingBox _bbox;
		};

		struct BoundingSphereCB : public osg::Drawable::ComputeBoundingSphereCallback
		{
			BoundingSphereCB() {}
			BoundingSphereCB(const osg::BoundingSphere &bs) : _bounds(bs) {};
			osg::BoundingSphere computeBound(const osg::Node&) const { return _bounds; }
		private:
			osg::BoundingSphere _bounds;
		};
	public:
		MeshLayerGenerator(const MeshLayerConfig &mesh_data)
		{
			//const bool useMultiDrawArraysIndirect = true;
			//m_GpuData.setUseMultiDrawArraysIndirect(useMultiDrawArraysIndirect);
			//_SetupGPUData(mesh_data);
			GPUCullData* gpuData = _CreateGPUData(mesh_data);
			m_TerrainStateSet = _CreateTerrainStateSet(gpuData);
			osg::BoundingSphere bs;
			m_InstanceGroup = _CreateInstanceGroup(gpuData);
			delete gpuData;
		}

		osg::Group* CreateMeshTile(osg::Geometry* terrain_geometry)
		{
			osg::Group* group = new osg::Group();
			group->getOrCreateStateSet()->setRenderBinDetails(1, "TraversalOrderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
			osg::Geode* terrain_geode = new osg::Geode();
			terrain_geode->addDrawable(terrain_geometry);
			terrain_geode->setStateSet(m_TerrainStateSet);
			DrawCallbackVisitor scbv;
			terrain_geometry->accept(scbv);
			//terrain_geometry->setDrawCallback(new TerrainGeometryDrawCB(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
			group->addChild(terrain_geode);
			group->addChild(m_InstanceGroup);
			//osg::BoundingSphere bs = terrain_geode->getBound();
			//osg::ref_ptr<osg::Group> instance_group = _CreateInstanceGroup(bs);
			//group->addChild(instance_group);


			return group;
		}

		osg::Group* CreateMeshNode(osg::Node* terrain_geometry)
		{
			osg::Group* group = new osg::Group();
			group->getOrCreateStateSet()->setRenderBinDetails(1, "TraversalOrderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
			osg::Group* terrain_geode = new osg::Group();
			terrain_geode->addChild(terrain_geometry);
			terrain_geode->setStateSet(m_TerrainStateSet);
			DrawCallbackVisitor scbv;
			terrain_geometry->accept(scbv);
			group->addChild(terrain_geode);
			group->addChild(m_InstanceGroup);
			//osg::BoundingSphere bs = terrain_geode->getBound();
			//osg::ref_ptr<osg::Group> instance_group = _CreateInstanceGroup(bs);
			//group->addChild(instance_group);
			return group;
		}
	private:
#if 0
		void _SetupGPUData(const MeshTileGeneratorConfig &mesh_data)
		{
			osg::ref_ptr < osg::Program> drawProgram = new osg::Program;
			//stateset->setAttribute(program, osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
			drawProgram->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_mesh_render_vertex.glsl")));
			drawProgram->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_mesh_render_fragment.glsl")));
			m_GpuData.registerIndirectTarget(0, new AggregateGeometryVisitor(new ConvertTrianglesOperatorClassic()), drawProgram);
			//m_GpuData.registerIndirectTarget(1, new AggregateGeometryVisitor(new ConvertTrianglesOperatorClassic()), drawProgram);

			for (size_t i = 0; i < mesh_data.MeshTypes.size(); i++)
			{
				for (size_t j = 0; j < mesh_data.MeshTypes[i].MeshLODs.size(); j++)
				{
#if 1
					osg::ref_ptr<osg::Node> mesh = osgDB::readNodeFile(mesh_data.MeshTypes[i].MeshLODs[j].Mesh);
#else
					osg::ref_ptr<osg::Node> mesh;
					if (mesh_data[i].MeshLODs[j].Mesh == "LOD0") mesh = createConiferTree(0.75f, osg::Vec4(1.0, 1.0, 1.0, 1.0), osg::Vec4(0.0, 1.0, 0.0, 1.0));
					else if (mesh_data[i].MeshLODs[j].Mesh == "LOD1") mesh = createConiferTree(0.45f, osg::Vec4(0.0, 0.0, 1.0, 1.0), osg::Vec4(1.0, 1.0, 0.0, 1.0));
					else if (mesh_data[i].MeshLODs[j].Mesh == "LOD2") mesh = createConiferTree(0.15f, osg::Vec4(1.0, 0.0, 0.0, 1.0), osg::Vec4(0.0, 0.0, 1.0, 1.0));
					else mesh = osgDB::readNodeFile(mesh_data[i].MeshLODs[j].Mesh);
#endif
					m_GpuData.registerType(i, 0, mesh.get(), mesh_data.MeshTypes[i].MeshLODs[j].Distance, mesh_data.Density);
				}
			}

			// every target will store 6 rows of data in GL_RGBA32F_ARB format ( the whole StaticInstance structure )
			m_GpuData.endRegister(6, GL_RGBA, GL_FLOAT, GL_RGBA32F_ARB);

			// in the end - we create OSG objects that draw instances using indirect targets and commands.
			std::map<unsigned int, IndirectTarget>::iterator it, eit;
			for (it = m_GpuData.targets.begin(), eit = m_GpuData.targets.end(); it != eit; ++it)
			{
				//it->second.geometryAggregator->getAggregatedGeometry()->setDrawCallback(new InvokeMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
				it->second._geometryAggregator->getAggregatedGeometry()->setDrawCallback(new InstanceGeometryDrawCB(it->second._indirectCommandTextureBuffer, it->second._indirectCommandImageBinding));
				it->second._geometryAggregator->getAggregatedGeometry()->setComputeBoundingBoxCallback(new BoundingBoxCB());
				it->second._geometryAggregator->getAggregatedGeometry()->setCullingActive(false);
			}
		}
#endif

		GPUCullData* _CreateGPUData(const MeshLayerConfig &mesh_data)
		{
			GPUCullData* gpuData = new GPUCullData();
			const bool useMultiDrawArraysIndirect = true;
			gpuData->setUseMultiDrawArraysIndirect(useMultiDrawArraysIndirect);

			osg::ref_ptr < osg::Program> drawProgram = new osg::Program;
			//stateset->setAttribute(program, osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
			drawProgram->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_mesh_render_vertex.glsl")));
			drawProgram->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_mesh_render_fragment.glsl")));
			gpuData->registerIndirectTarget(0, new AggregateGeometryVisitor(new ConvertTrianglesOperatorClassic()), drawProgram);
			//m_GpuData.registerIndirectTarget(1, new AggregateGeometryVisitor(new ConvertTrianglesOperatorClassic()), drawProgram);

			for (size_t i = 0; i < mesh_data.MeshTypes.size(); i++)
			{
				for (size_t j = 0; j < mesh_data.MeshTypes[i].MeshLODs.size(); j++)
				{
#if 1
					osg::ref_ptr<osg::Node> mesh = osgDB::readNodeFile(mesh_data.MeshTypes[i].MeshLODs[j].Mesh);
#else
					osg::ref_ptr<osg::Node> mesh;
					if (mesh_data[i].MeshLODs[j].Mesh == "LOD0") mesh = createConiferTree(0.75f, osg::Vec4(1.0, 1.0, 1.0, 1.0), osg::Vec4(0.0, 1.0, 0.0, 1.0));
					else if (mesh_data[i].MeshLODs[j].Mesh == "LOD1") mesh = createConiferTree(0.45f, osg::Vec4(0.0, 0.0, 1.0, 1.0), osg::Vec4(1.0, 1.0, 0.0, 1.0));
					else if (mesh_data[i].MeshLODs[j].Mesh == "LOD2") mesh = createConiferTree(0.15f, osg::Vec4(1.0, 0.0, 0.0, 1.0), osg::Vec4(0.0, 0.0, 1.0, 1.0));
					else mesh = osgDB::readNodeFile(mesh_data[i].MeshLODs[j].Mesh);
#endif
					gpuData->registerType(i, 0, mesh.get(), mesh_data.MeshTypes[i].MeshLODs[j].Distance, mesh_data.Density);
				}
			}

			// every target will store 6 rows of data in GL_RGBA32F_ARB format ( the whole StaticInstance structure )
			gpuData->endRegister(6, GL_RGBA, GL_FLOAT, GL_RGBA32F_ARB);

			// in the end - we create OSG objects that draw instances using indirect targets and commands.
			std::map<unsigned int, IndirectTarget>::iterator it, eit;
			for (it = gpuData->targets.begin(), eit = gpuData->targets.end(); it != eit; ++it)
			{
				//it->second.geometryAggregator->getAggregatedGeometry()->setDrawCallback(new InvokeMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
				it->second._geometryAggregator->getAggregatedGeometry()->setDrawCallback(new InstanceGeometryDrawCB(it->second._indirectCommandTextureBuffer, it->second._indirectCommandImageBinding));
				it->second._geometryAggregator->getAggregatedGeometry()->setComputeBoundingBoxCallback(new BoundingBoxCB());
				it->second._geometryAggregator->getAggregatedGeometry()->setCullingActive(false);
			}
			return gpuData;
		}

		osg::Group* _CreateInstanceGroup(GPUCullData* gpuData)
		{
			osg::Group* group = new osg::Group();
			// in the end - we create OSG objects that draw instances using indirect targets and commands.
			std::map<unsigned int, IndirectTarget>::iterator it, eit;
			for (it = gpuData->targets.begin(), eit = gpuData->targets.end(); it != eit; ++it)
			{
				osg::ref_ptr<osg::Geode> drawGeode = new osg::Geode;
				drawGeode->addDrawable(it->second._geometryAggregator->getAggregatedGeometry());
				//drawGeode->setCullingActive(false);
				//drawGeode->setComputeBoundingSphereCallback(new BoundingSphereCB(terrain_bs));
				it->second.addIndirectTargetData(false, "ov_indirectTarget", it->first, drawGeode->getOrCreateStateSet());
				drawGeode->getOrCreateStateSet()->setAttributeAndModes(gpuData->instanceTypesUBB.get());
				it->second.addDrawProgram("ov_instanceTypesData", drawGeode->getOrCreateStateSet());
				drawGeode->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
				//it->second.geometryAggregator->getAggregatedGeometry()->setComputeBoundingBoxCallback(new StaticBoundingBox());
				group->addChild(drawGeode);
			}
			return group;
		}

		osg::StateSet* _CreateTerrainStateSet(GPUCullData* gpuData)
		{
			osg::StateSet* terrain_ss = new osg::StateSet();
			// instance OSG tree is connected to cull shader with all necessary data ( all indirect commands, all
			// indirect targets, necessary OpenGl modes etc. )
			{
				//osg::ref_ptr<ResetTexturesCallback> resetTexturesCallback = new ResetTexturesCallback;
				osg::ref_ptr < osg::Program> cullProgram = new osg::Program;
				cullProgram->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("ov_mesh_cull_vertex.glsl")));
				cullProgram->addShader(osg::Shader::readShaderFile(osg::Shader::TESSCONTROL, osgDB::findDataFile("ov_mesh_cull_tess_ctrl.glsl")));
				cullProgram->addShader(osg::Shader::readShaderFile(osg::Shader::TESSEVALUATION, osgDB::findDataFile("ov_mesh_cull_tess_eval.glsl")));
				cullProgram->addShader(osg::Shader::readShaderFile(osg::Shader::GEOMETRY, osgDB::findDataFile("ov_mesh_cull_geometry.glsl")));
#if 0
				cullProgram->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("ov_mesh_cull_fragment.glsl")));
#else
				terrain_ss->setMode(GL_RASTERIZER_DISCARD, osg::StateAttribute::ON);
#endif
				cullProgram->addBindUniformBlock("ov_instanceTypesData", 1);
				terrain_ss->setAttributeAndModes(cullProgram.get(), osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
				terrain_ss->setAttributeAndModes(gpuData->instanceTypesUBB.get());

				std::map<unsigned int, IndirectTarget>::iterator it, eit;
				for (it = gpuData->targets.begin(), eit = gpuData->targets.end(); it != eit; ++it)
				{
					it->second.addIndirectCommandData("ov_indirectCommand", it->first, terrain_ss);
					//resetTexturesCallback->addTextureDirty(it->first);
					//resetTexturesCallback->addTextureDirtyParams(it->first);

					it->second.addIndirectTargetData(true, "ov_indirectTarget", it->first, terrain_ss);
					//resetTexturesCallback->addTextureDirtyParams(OSGGPUCULL_MAXIMUM_INDIRECT_TARGET_NUMBER + it->first);
				}

				osg::Uniform* indirectCommandSize = new osg::Uniform(osg::Uniform::INT, "ov_indirectCommandSize");
				indirectCommandSize->set((int)(sizeof(DrawArraysIndirectCommand) / sizeof(unsigned int)));
				terrain_ss->addUniform(indirectCommandSize);

				osg::Uniform* numInstanceTypesUniform = new osg::Uniform(osg::Uniform::INT, "ov_numInstanceTypes");
				int num_instance_types = static_cast<int>(gpuData->instanceTypes->getData().size());
				numInstanceTypesUniform->set(num_instance_types);
				terrain_ss->addUniform(numInstanceTypesUniform);
				//terrain_ss->setUpdateCallback(resetTexturesCallback.get());
			}
			return terrain_ss;
		}
	private:
		osg::ref_ptr<osg::StateSet>  m_TerrainStateSet;
		osg::ref_ptr<osg::Group> m_InstanceGroup;
	};
}