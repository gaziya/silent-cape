
const char* sVolumeMarch = R"(

cbuffer _ : register(b0)
{
	float2 R;
};
        
RWStructuredBuffer<float3x3> seq : register(u3);
RWStructuredBuffer<float4> obj : register(u8);
RWStructuredBuffer<float4> back : register(u5);
RWStructuredBuffer<float4> image : register(u6);

#define R(p,a,t) lerp(a*dot(p,a),p,cos(t))+sin(t)*cross(p,a)
#define H(h) (cos((h)*6.3+float3(0,23,21))*.5+.5)
	
float2 map(float3 p)
{
	p=asin(sin(p));
	float s=1.,e;
	for(int j=0;j++<7;)
	{
		s*=e=max(.1/dot(p,p),1.);
		p=R(abs(p.zxy)*e-.1,(float3).577,1.08);
	}
	return float2(p.x/s,s);
}
        
[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if( any( id >= uint2(R) ) )return;
	float2 uv = (float2(id*2)-R)/R.y;

    float3 rd=mul(seq[0],normalize(float3(uv,1.5)));
    float3 ro=seq[1][0];
	float t=back[(id.y/2) * uint(R.x) + id.x/2].x -.3;
	float4 rabit=obj[(id.y) * uint(R.x) + id.x];
	float3 O = (float3)0;
    for(float i=0.,e;i<50.;i++)
    {
    	float3 p=ro+rd*t-i/1e4;
    	float2 a=map(p);
    	t+=e=abs(a.x)+.0001;
    	O+=mul(seq[2],H(log(a.y)*.2)*.03/exp(i*i*e)); 
    	if(t>rabit.w)
    	{
    		O+= rabit.xyz/exp(20.*dot(O,O));
    		break;
    	}
    }
	image[id.y * uint(R.x) + id.x].xyz = O*O;
}
    )";

