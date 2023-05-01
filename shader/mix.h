// -------------------------------------------
//
// .......... Screen Output < MIX > ... (u0)
// -------------------------------------------
const char* sMix = R"(

cbuffer _:register(b0)
{
	float2 R;
	float time;
};

RWStructuredBuffer<float4> screen : register(u0);

RWStructuredBuffer<float4> u4  : register(u4);
RWStructuredBuffer<float4> u5  : register(u5);
RWStructuredBuffer<float4> u6  : register(u6);
RWStructuredBuffer<float4> u7  : register(u7);
RWStructuredBuffer<float4> u8  : register(u8);

[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if ( all(id >= uint2(R)) ) return;
	uint ID = id.y * uint(R.x) + id.x;
	screen[ID] = (float4)0;
	screen[ID] += u6[ID] * smoothstep(2.,6.,time) * smoothstep(78.,73.,time);
	screen[ID] += u7[ID];
}

)";
