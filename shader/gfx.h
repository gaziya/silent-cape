
#include "randomTexture.h"
#include "sdfTexture.h"
#include "front.h"
#include "mix.h"
#include "seq.h"
#include "preSdf.h"
#include "volumeMarch.h"
#include "preBB.h"
#include "objMarch.h"


#define  CompShaderCnt  9

const char* sDummy = R"(
[numthreads(1, 1, 1)]
void CSMain(uint2 id : SV_DispatchThreadID){}
)";



const char* CompShaderSrc(UINT n)
{
	switch (n)
	{
		case 0: return sRamdamTexture;
		case 1: return sSdfTexture;
		case 2: return sSeq;
		case 3: return sPreBB;
		case 4: return sPreSdf;
		case 5: return sVolumeMarch;
		case 6: return sFront;
		case 7: return sMix;
		case 8: return sObjMarch;
    }
	const char* ret = "";
	return ret;
}
    

// -------------------------------------------------------------
// ++++++++ MAIN ++++++ for DISPLAY ++++++++++++++++++++++
//                                    << Fixed >>  
// ------------------------------------------------

const char* sGfx =  R"(
cbuffer CB : register(b0)
{
	float2 resolution;
};
    
StructuredBuffer<float4> image: register(t0); 

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(uint id :SV_VertexID)
{
	PSInput result;
	int g = abs(3-int(id)%6);
	result.uv = float2(g>>1, g&1);
	result.position = float4(result.uv*2.-1., 0 ,1);
	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	uint2 p = int2(input.uv * resolution);
	return image[p.y * uint(resolution.x) + p.x];
}
)";



