// -------------------------------------------
//
// ........ Scene 02 < Font Texture Test > ..(u7)
//
// -------------------------------------------
const char* sFront = R"(
cbuffer _:register(b0)
{
	float2 R;
	float time;
};
        
RWStructuredBuffer<float4> sdfFont : register(u2);

RWStructuredBuffer<float4> image : register(u7);

uint uv2id(float2 uv, int2 storage)
{
	uint2 id = min(uint2(max((uv*.5+.5)*512, 0.)),(uint2)511);
	return id.y * 0x800u + id.x + storage.x * 0x200u + storage.y * 0x100000u;
}
float deFont(float2 uv, uint no)
{
	uint3 storage = uint3(no/16, (no/4)%4, no%4);
	return sdfFont[uv2id(uv, storage.xy)][storage.z];
}

#define Q(a,b)q.x-=a;de=min(de,deFont(q,b));
[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if( any( id >= uint2(R) ) )return;
	uint ID = id.y * uint(R.x) + id.x;
	image[ID] = (float4)0;
	float2 uv = (float2(id*2)-R)/R.y,q;
	float de;
	
	q=uv*6.;
	de=1.;
	Q(-6.,18)
	Q(.8,8)
	Q(1.,11)
	Q(1.3,4)
	Q(1.3,13)
	Q(1.3,19)
	Q(2.,2)
	Q(1.5,0)
	Q(1.5,15)
	Q(1.4,4)
	image[ID] += (float4)smoothstep(.03, .0, de) * smoothstep(10.,8.,time);
	
	
	de=1.;
	uv.x-=-.2;
	q=uv*12.;
	Q(0.,6)
	Q(1.3,0)
	Q(1.4,25)
	image[ID] += (float4)smoothstep(.03, .0, de) * smoothstep(70.,75.,time);
}


)";
	
