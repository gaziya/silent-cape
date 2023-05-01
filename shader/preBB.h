


const char* sPreBB = R"(

cbuffer _:register(b0)
{
	float2 resolution;
};

        
RWStructuredBuffer<float3x3> seq : register(u3);
RWStructuredBuffer<float4> image : register(u4);

float bb(float3 d, float3 p, float r)
{
	float3 v=cross(p, d);
	float l = dot(v,v);
	float a = r*r-l;
	if (a<0) return a;
	return sqrt(dot(p,p) - l) - sqrt(a);
}

[numthreads(16, 16, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if( any( id >= uint2(resolution/2.) ) )return;
	float2 r = resolution/2.;
	float2 uv = (float2(id*2)-r)/r.y;

    float3 rd=mul(seq[0],normalize(float3(uv,2.)));
	float3 ro=seq[1][0];
	float z=100.;
	float4 O = (float4)z;
	for(int j=0;j<30;j++)
	{
		float3 a=seq[10+j][0];
		float t =bb(rd,ro-a,.1);
		if(t>=0. && z>t)
    	{
    		z=t;
    		O=(float4)t;
    	}
    }	
	image[id.y * uint(resolution.x) + id.x] = O;
}

)";

