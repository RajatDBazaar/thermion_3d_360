#pragma once

#include "Log.hpp"

#include <chrono>
#include <variant>

#include <filament/Engine.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/TransformManager.h>

#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat3.h>
#include <math/norm.h>

#include <gltfio/Animator.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/ResourceLoader.h>
#include <utils/NameComponentManager.h>

template class std::vector<float>;
namespace flutter_filament
{
    using namespace filament;
    using namespace filament::gltfio;
    using namespace utils;
    using namespace std::chrono;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_t;

    enum AnimationType
    {
        MORPH,
        BONE,
        GLTF
    };

    struct AnimationStatus
    {
        time_point_t start = time_point_t::max();
        bool loop = false;
        bool reverse = false;
        float durationInSecs = 0;
    };

    struct GltfAnimation : AnimationStatus
    {
        int index = -1;
    };

    //
    // Use this to construct a dynamic (i.e. non-glTF embedded) morph target animation.
    //
    struct MorphAnimation : AnimationStatus
    {
        utils::Entity meshTarget;
        int numFrames = -1;
        float frameLengthInMs = 0;
        std::vector<float> frameData;
        std::vector<int> morphIndices;
        int lengthInFrames;
    };

    //
    // Use this to construct a dynamic (i.e. non-glTF embedded) bone/joint animation.
    //
    struct BoneAnimation : AnimationStatus
    {
        size_t boneIndex;
        size_t skinIndex = 0;
        int lengthInFrames;
        float frameLengthInMs = 0;
        std::vector<math::mat4f> frameData;
    };

    struct AnimationComponent
    {
        std::variant<FilamentInstance *, Entity> target;
        std::vector<GltfAnimation> gltfAnimations;
        std::vector<MorphAnimation> morphAnimations;
        std::vector<BoneAnimation> boneAnimations;

        // the index of the last active glTF animation,
        // used to cross-fade
        int fadeGltfAnimationIndex = -1;
        float fadeDuration = 0.0f;
        float fadeOutAnimationStart = 0.0f;
    };

    class AnimationComponentManager : public utils::SingleInstanceComponentManager<AnimationComponent>
    {

        filament::TransformManager &_transformManager;
        filament::RenderableManager &_renderableManager;

    public:
        AnimationComponentManager(
            filament::TransformManager &transformManager,
            filament::RenderableManager &renderableManager) : _transformManager(transformManager),
                                                              _renderableManager(renderableManager){};

        void addAnimationComponent(std::variant<FilamentInstance *, Entity> target)
        {
            AnimationComponent animationComponent;
            animationComponent.target = target;
            EntityInstanceBase::Type componentInstance;
            if (std::holds_alternative<FilamentInstance *>(target))
            {
                auto instance = std::get<FilamentInstance *>(target);
                componentInstance = addComponent(instance->getRoot());
            }
            else
            {
                componentInstance = addComponent(std::get<Entity>(target));
            }

            this->elementAt<0>(componentInstance) = animationComponent;
        }


        void removeAnimationComponent(std::variant<FilamentInstance *, Entity> target)
        {
            AnimationComponent animationComponent;
            animationComponent.target = target;
            EntityInstanceBase::Type componentInstance;
            if (std::holds_alternative<FilamentInstance *>(target))
            {
                auto instance = std::get<FilamentInstance *>(target);
                if(hasComponent(instance->getRoot())) {
                    removeComponent(instance->getRoot());
                }
            } else {
                auto entity = std::get<Entity>(target);
                if(hasComponent(entity)) {
                    removeComponent(entity);
                }
            }
        }

        void update()
        {
            auto now = high_resolution_clock::now();

            for (auto it = begin(); it < end(); it++)
            {
                const auto &entity = getEntity(it);

                auto componentInstance = getInstance(entity);
                auto &animationComponent = elementAt<0>(componentInstance);

                auto &morphAnimations = animationComponent.morphAnimations;

                if (std::holds_alternative<FilamentInstance *>(animationComponent.target))
                {
                    auto target = std::get<FilamentInstance *>(animationComponent.target);
                    auto animator = target->getAnimator();
                    auto &gltfAnimations = animationComponent.gltfAnimations;
                    auto &boneAnimations = animationComponent.boneAnimations;

                    if(gltfAnimations.size() > 0) {
                        for (int i = ((int)gltfAnimations.size()) - 1; i >= 0; i--)
                        {

                            auto animationStatus = animationComponent.gltfAnimations[i];

                            auto elapsedInSecs = float(std::chrono::duration_cast<std::chrono::milliseconds>(now - animationStatus.start).count()) / 1000.0f;

                            if (!animationStatus.loop && elapsedInSecs >= animationStatus.durationInSecs)
                            {
                                animator->applyAnimation(animationStatus.index, animationStatus.durationInSecs - 0.001);
                                animator->updateBoneMatrices();
                                gltfAnimations.erase(gltfAnimations.begin() + i);
                                animationComponent.fadeGltfAnimationIndex = -1;
                                continue;
                            }
                            animator->applyAnimation(animationStatus.index, elapsedInSecs);

                            if (animationComponent.fadeGltfAnimationIndex != -1 && elapsedInSecs < animationComponent.fadeDuration)
                            {
                                // cross-fade
                                auto fadeFromTime = animationComponent.fadeOutAnimationStart + elapsedInSecs;
                                auto alpha = elapsedInSecs / animationComponent.fadeDuration;
                                animator->applyCrossFade(animationComponent.fadeGltfAnimationIndex, fadeFromTime, alpha);
                            }
                        }

                        animator->updateBoneMatrices();
                    }

                    for (int i = (int)boneAnimations.size() - 1; i >= 0; i--)
                    {
                        auto animationStatus = boneAnimations[i];

                        auto elapsedInSecs = float(std::chrono::duration_cast<std::chrono::milliseconds>(now - animationStatus.start).count()) / 1000.0f;

                        if (!animationStatus.loop && elapsedInSecs >= animationStatus.durationInSecs)
                        {
                            Log("Bone animation %d finished", i);
                            boneAnimations.erase(boneAnimations.begin() + i);
                            continue;
                        }

                        float elapsedFrames = elapsedInSecs * 1000.0f / animationStatus.frameLengthInMs;

                        int currFrame = static_cast<int>(elapsedFrames) % animationStatus.lengthInFrames;
                        float delta = elapsedFrames - currFrame;
                        int nextFrame = currFrame;

                        // offset from the end if reverse
                        if (animationStatus.reverse)
                        {
                            currFrame = animationStatus.lengthInFrames - currFrame;
                            if (currFrame > 0)
                            {
                                nextFrame = currFrame - 1;
                            }
                            else
                            {
                                nextFrame = 0;
                            }
                        }
                        else
                        {
                            if (currFrame < animationStatus.lengthInFrames - 1)
                            {
                                nextFrame = currFrame + 1;
                            }
                            else
                            {
                                nextFrame = currFrame;
                            }
                        }

                        // simple linear interpolation
                        math::mat4f curr = (1 - delta) * animationStatus.frameData[currFrame];
                        math::mat4f next = delta * animationStatus.frameData[nextFrame];
                        math::mat4f localTransform = curr + next;

                        const Entity joint = target->getJointsAt(animationStatus.skinIndex)[animationStatus.boneIndex];

                        auto jointTransform = _transformManager.getInstance(joint);

                        _transformManager.setTransform(jointTransform, localTransform);

                        animator->updateBoneMatrices();

                        if (animationStatus.loop && elapsedInSecs >= animationStatus.durationInSecs)
                        {
                            animationStatus.start = now;
                        }
                    }
                }
                for (int i = (int)morphAnimations.size() - 1; i >= 0; i--)
                {

                    auto animationStatus = morphAnimations[i];

                    auto elapsedInSecs = float(std::chrono::duration_cast<std::chrono::milliseconds>(now - animationStatus.start).count()) / 1000.0f;

                    if (!animationStatus.loop && elapsedInSecs >= animationStatus.durationInSecs)
                    {
                        morphAnimations.erase(morphAnimations.begin() + i);
                        continue;
                    }

                    int frameNumber = static_cast<int>(elapsedInSecs * 1000.0f / animationStatus.frameLengthInMs) % animationStatus.lengthInFrames;
                    // offset from the end if reverse
                    if (animationStatus.reverse)
                    {
                        frameNumber = animationStatus.lengthInFrames - frameNumber;
                    }
                    auto baseOffset = frameNumber * animationStatus.morphIndices.size();
                    for (int i = 0; i < animationStatus.morphIndices.size(); i++)
                    {
                        auto morphIndex = animationStatus.morphIndices[i];
                        // set the weights appropriately
                        _renderableManager.setMorphWeights(
                            _renderableManager.getInstance(animationStatus.meshTarget),
                            animationStatus.frameData.data() + baseOffset + i,
                            1,
                            morphIndex);
                    }
                }
            }
        }
    };
}
