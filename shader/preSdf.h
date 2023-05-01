
const char* sPreSdf = R"(

cbuffer _:register(b0)
{
	float2 R;
};
        
RWStructuredBuffer<float3x3> seq : register(u3);
RWStructuredBuffer<float4> image : register(u5);

#define R(p,a,t) lerp(a*dot(p,a),p,cos(t))+sin(t)*cross(p,a)
    
float map(float3 p)
{
	p=asin(sin(p));
	float s=1.,e;
	for(int j=0;j++<7;)
	{
		s*=e=max(.1/dot(p,p),1.);
		p=R(abs(p.zxy)*e-.1,(float3).577,1.08);
	}
return p.x/s;
}

        
[numthreads(16, 16, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if( any( id >= uint2(R/2.) ) )return;
	float2 uv = (float2(id*2)-R/2.)/R.y*2.;

	float3 rd=mul(seq[0],normalize(float3(uv,1.5)));
	float3 ro=seq[1][0];
	float maxd=100.;
		
	float t=0.3,d;
	for(int i=0;i<80;i++)
	{
		float3 p=ro+rd*t;
		t+=d=map(p);
		if (t>maxd || d<.001) break;
	}
	image[id.y * uint(R.x) + id.x] = (float4)t;
}

        
    )";

