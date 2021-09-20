#ifndef XR_COMPOSITION_LAYER_PROVIDER_H
#define XR_COMPOSITION_LAYER_PROVIDER_H

#include "openxr/include/openxr_inc.h"

// Interface for OpenXR extensions that provide a composition layer.
class XRCompositionLayerProvider {
public:
	virtual XrCompositionLayerBaseHeader *get_composition_layer() = 0;
};

#endif // XR_COMPOSITION_LAYER_PROVIDER_H
