
// -------------------------------------------
//
//  ......... Ramdam Texture ........... (u1)
// -------------------------------------------


const char* sRamdamTexture = R"(

cbuffer _ : register(b0)
{
	float2 resolution;
};
        
RWStructuredBuffer<float4> im : register(u1);

float hash(uint n)
{
	n = (n << 13) ^ n; 
	uint m = n;
	n = n * 15731u;
	n = n * m;
	n = n + 789221u;
	n = n * m;
	n = n + 1376312589u; 
	n = (n>>14) & 65535u;
	return (float(n)/65535.)*2.-1.;
}

[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	uint ID = id.y * 2048u + id.x;
	im[ID] = float4(hash(ID*4+0),hash(ID*4+1),hash(ID*4+2),hash(ID*4+3));
}

)";
