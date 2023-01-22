#include "../../../DiligentTools/TextureLoader/interface/TextureUtilities.h"
#include "../../../DiligentFX/Components/interface/ShadowMapManager.hpp"

#include "../../../assets/BasicStructures.fxh"

struct ShadowSettings
{
	bool           SnapCascades = true;
	bool           StabilizeExtents = true;
	bool           EqualizeExtents = true;
	bool           SearchBestCascade = true;
	bool           FilterAcrossCascades = true;
	int            Resolution = 2048;
	float          PartitioningFactor = 0.95f;
	Diligent::TEXTURE_FORMAT Format = Diligent::TEX_FORMAT_D16_UNORM;
	int            iShadowMode = SHADOW_MODE_EVSM4;

	bool Is32BitFilterableFmt = true;
};