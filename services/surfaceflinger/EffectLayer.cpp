/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

// #define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "EffectLayer"

#include "EffectLayer.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <compositionengine/CompositionEngine.h>
#include <compositionengine/LayerFECompositionState.h>
#include <renderengine/RenderEngine.h>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include "DisplayDevice.h"
#include "SurfaceFlinger.h"

namespace android {
// ---------------------------------------------------------------------------

EffectLayer::EffectLayer(const LayerCreationArgs& args)
      : Layer(args),
        mCompositionState{mFlinger->getCompositionEngine().createLayerFECompositionState()} {}

EffectLayer::~EffectLayer() = default;

std::optional<compositionengine::LayerFE::LayerSettings> EffectLayer::prepareClientComposition(
        compositionengine::LayerFE::ClientCompositionTargetSettings& targetSettings) {
    auto result = Layer::prepareClientComposition(targetSettings);
    if (!result) {
        return result;
    }
    result->source.solidColor = getColor().rgb;
    return result;
}

bool EffectLayer::isVisible() const {
    return !isHiddenByPolicy() && getAlpha() > 0.0_hf;
}

bool EffectLayer::setColor(const half3& color) {
    if (mCurrentState.color.r == color.r && mCurrentState.color.g == color.g &&
        mCurrentState.color.b == color.b) {
        return false;
    }

    mCurrentState.sequence++;
    mCurrentState.color.r = color.r;
    mCurrentState.color.g = color.g;
    mCurrentState.color.b = color.b;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool EffectLayer::setDataspace(ui::Dataspace dataspace) {
    if (mCurrentState.dataspace == dataspace) {
        return false;
    }

    mCurrentState.sequence++;
    mCurrentState.dataspace = dataspace;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

void EffectLayer::preparePerFrameCompositionState() {
    Layer::preparePerFrameCompositionState();

    auto* compositionState = editCompositionState();
    compositionState->color = getColor();
    compositionState->compositionType = Hwc2::IComposerClient::Composition::SOLID_COLOR;
}

sp<compositionengine::LayerFE> EffectLayer::getCompositionEngineLayerFE() const {
    return asLayerFE();
}

compositionengine::LayerFECompositionState* EffectLayer::editCompositionState() {
    return mCompositionState.get();
}

const compositionengine::LayerFECompositionState* EffectLayer::getCompositionState() const {
    return mCompositionState.get();
}

bool EffectLayer::isOpaque(const Layer::State& s) const {
    // Consider the layer to be opaque if its opaque flag is set or its effective
    // alpha (considering the alpha of its parents as well) is 1.0;
    return (s.flags & layer_state_t::eLayerOpaque) != 0 || getAlpha() == 1.0_hf;
}

ui::Dataspace EffectLayer::getDataSpace() const {
    return mDrawingState.dataspace;
}

sp<Layer> EffectLayer::createClone() {
    sp<EffectLayer> layer = mFlinger->getFactory().createEffectLayer(
            LayerCreationArgs(mFlinger.get(), nullptr, mName + " (Mirror)", 0, 0, 0,
                              LayerMetadata()));
    layer->setInitialValuesForClone(this);
    return layer;
}

} // namespace android

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"