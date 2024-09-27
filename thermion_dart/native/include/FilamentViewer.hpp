#pragma once

#include <filament/Camera.h>
#include <filament/Frustum.h>
#include <filament/ColorGrading.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/LightManager.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/ResourceLoader.h>

#include <camutils/Manipulator.h>

#include <utils/NameComponentManager.h>

#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat3.h>
#include <math/norm.h>

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "ResourceBuffer.hpp"
#include "SceneManager.hpp"
#include "ThreadPool.hpp"

namespace thermion_filament
{

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_t;

    using namespace std::chrono;
    using namespace gltfio;
    using namespace camutils;

    enum ToneMapping
    {
        ACES,
        FILMIC,
        LINEAR
    };

    class FilamentViewer
    {

        typedef int32_t EntityId;

    public:
        FilamentViewer(const void *context, const ResourceLoaderWrapperImpl *const resourceLoaderWrapper, void *const platform = nullptr, const char *uberArchivePath = nullptr);
        ~FilamentViewer();

        void setToneMapping(ToneMapping toneMapping);
        void setBloom(float strength);
        void loadSkybox(const char *const skyboxUri);
        void removeSkybox();

        void loadIbl(const char *const iblUri, float intensity);
        void removeIbl();
        void rotateIbl(const math::mat3f &matrix);
        void createIbl(float r, float g, float b, float intensity);

        void removeEntity(EntityId asset);
        void clearEntities();

        void updateViewport(uint32_t width, uint32_t height);
        bool render(
            uint64_t frameTimeInNanos,
            void *pixelBuffer,
            void (*callback)(void *buf, size_t size, void *data),
            void *data);
        void setFrameInterval(float interval);

        bool setCamera(EntityId asset, const char *nodeName);
        void setMainCamera();
        EntityId getMainCamera();
        Camera* getCamera(EntityId entity);
        
        float getCameraFov(bool horizontal);
        void setCameraFov(double fovDegrees, bool horizontal);

        void createSwapChain(const void *surface, uint32_t width, uint32_t height);
        void destroySwapChain();

        void createRenderTarget(intptr_t textureId, uint32_t width, uint32_t height);

        Renderer *getRenderer();

        void setBackgroundColor(const float r, const float g, const float b, const float a);
        void setBackgroundImage(const char *resourcePath, bool fillHeight);
        void clearBackgroundImage();
        void setBackgroundImagePosition(float x, float y, bool clamp);

        void setViewFrustumCulling(bool enabled);

        void setCameraManipulatorOptions(filament::camutils::Mode mode, double orbitSpeedX, double orbitSpeedY, double zoomSpeed);
        void grabBegin(float x, float y, bool pan);
        void grabUpdate(float x, float y);
        void grabEnd();
        void scrollBegin();
        void scrollUpdate(float x, float y, float delta);
        void scrollEnd();
        void pick(uint32_t x, uint32_t y, void (*callback)(EntityId entityId, int x, int y));
        Engine* getEngine() { 
            return _engine;
        }

        EntityId addLight(
            LightManager::Type t, 
            float colour, 
            float intensity, 
            float posX, 
            float posY, 
            float posZ, 
            float dirX, 
            float dirY, 
            float dirZ, 
            float falloffRadius,
            float spotLightConeInner,
            float spotLightConeOuter,
            float sunAngularRadius,
            float sunHaloSize,
            float sunHaloFallof,
            bool shadows);
        void setLightPosition(EntityId entityId, float x, float y, float z);
        void setLightDirection(EntityId entityId, float x, float y, float z);
        void removeLight(EntityId entityId);
        void clearLights();
        void setPostProcessing(bool enabled);

        void setRecording(bool recording);
        void setRecordingOutputDirectory(const char *path);
        void capture(uint8_t *out, bool useFence, void (*onComplete)());

        void setAntiAliasing(bool msaaEnabled, bool fxaaEnabled, bool taaEnabled);
        void setDepthOfField();
        void setShadowsEnabled(bool enabled);
        void setShadowType(ShadowType shadowType);
        void setSoftShadowOptions( float penumbraScale, float penumbraRatioScale);

        SceneManager *const getSceneManager()
        {
            return (SceneManager *const)_sceneManager;
        }

        void unprojectTexture(EntityId entity, uint8_t* input, uint32_t inputWidth, uint32_t inputHeight, uint8_t* out, uint32_t outWidth, uint32_t outHeight);  

    private:
        const ResourceLoaderWrapperImpl *const _resourceLoaderWrapper;
        void* _context = nullptr;
        Scene *_scene = nullptr;
        View *_view = nullptr;       
        Engine *_engine = nullptr;
        thermion_filament::ThreadPool *_tp = nullptr;
        Renderer *_renderer = nullptr;
        SwapChain *_swapChain = nullptr;
        SceneManager *_sceneManager = nullptr;

        std::mutex mtx; // mutex to ensure thread safety when removing assets

        std::vector<utils::Entity> _lights;
        Texture *_skyboxTexture = nullptr;
        Skybox *_skybox = nullptr;
        Texture *_iblTexture = nullptr;
        IndirectLight *_indirectLight = nullptr;

        float _frameInterval = 1000.0 / 60.0;

        // Camera properties
        Camera *_mainCamera = nullptr; // the default camera added to every scene. If you want the *active* camera, access via View.
        
        Manipulator<double> *_manipulator = nullptr;
        filament::camutils::Mode _manipulatorMode = filament::camutils::Mode::ORBIT;
        double _orbitSpeedX = 0.01;
        double _orbitSpeedY = 0.01;
        double _zoomSpeed = 0.01;
        
        void _createManipulator();

        ColorGrading *colorGrading = nullptr;

        // background image properties
        uint32_t _imageHeight = 0;
        uint32_t _imageWidth = 0;
        filament::math::mat4f _imageScale;
        Texture *_imageTexture = nullptr;
        Texture *_dummyImageTexture = nullptr;
        utils::Entity _imageEntity;
        VertexBuffer *_imageVb = nullptr;
        IndexBuffer *_imageIb = nullptr;
        Material *_imageMaterial = nullptr;
        TextureSampler _imageSampler;
        void loadKtx2Texture(std::string path, ResourceBuffer data);
        void loadKtxTexture(std::string path, ResourceBuffer data);
        void loadPngTexture(std::string path, ResourceBuffer data);
        void loadTextureFromPath(std::string path);
        void savePng(void *data, size_t size, int frameNumber);
        void createBackgroundImage();

        time_point_t _recordingStartTime = std::chrono::high_resolution_clock::now();
        time_point_t _fpsCounterStartTime = std::chrono::high_resolution_clock::now();

        bool _recording = false;
        std::string _recordingOutputDirectory = std::string("/tmp");
        std::mutex _recordingMutex;
        std::mutex _imageMutex;
        double _cumulativeAnimationUpdateTime = 0;
        int _frameCount = 0;
        int _skippedFrames = 0;
    };

    struct FrameCallbackData
    {
        FilamentViewer *viewer;
        uint32_t frameNumber;
    };

}
