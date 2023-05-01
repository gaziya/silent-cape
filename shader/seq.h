// -------------------------------------------
//
//  ......... Seqence ............. (u3)
// -------------------------------------------
const char* sSeq = R"(


cbuffer _:register(b0)
{
	float2 resolution;
	float time;
};
        
RWStructuredBuffer<float4> hash : register(u1);
RWStructuredBuffer<float3x3> seq : register(u3);

#define U(a)asuint(a)%0x400000
#define E(a)float3(cos(a.y)*cos(a.x),sin(a.y),cos(a.y)*sin(a.x))

float3x3 lookat(float3 w){
    float3 u=cross(w,float3(0,1,0));
	return transpose(float3x3(u,cross(u,w),w));
}
	
float3 randCurve(float t,float n)
{
	float3 p = (float3)0;
	for (int i=0; i<6; i++)
	{
		p += E(hash[U(n)])*sin(t+sin(t*1.6*hash[U(n)].w)*0.5);
		n += 165.0;
		t *= 0.8;
	}
	return p;
}



#define A(a)T=smoothstep(0.,1.,clamp((time-SAM)/a, 0.0, 1.0));SAM+=a;


	
[numthreads(1, 1, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if ( all( id >(uint2)0 ) ) return;


	float T,SAM=0.0;
	
	float2 dir[4] ={
    	float2(-2,.4),
		float2(2,-.8),
		float2(-2,.0),
		float2(-1,.4)
	};

	float2 b=dir[0];
	for(int i=0;i<30;i++)
	{
		A(3.0)
		A(3.0)
		b=lerp(b,dir[i&3],T);
	}

	float3 pnt[4]={
		float3(3,4,6),
		float3(6.5,3.8,6),
		float3(-3.5,8,-3),
		float3(20.6,-4,6)
	};
    

	SAM=0.0;
	float3 k=(float3)0;
	for(int i=0;i<30;i++)
	{
		A(4.0)
		A(6.0)
		k=lerp(k,pnt[i&3],T);
	}
	
	float3x3 c[4] = {
		float3x3(.7,0,0,.8,0,0,.8,0,0),
		float3x3(1,0,0,0,1,0,0,0,1),
		float3x3(0,0,0,0,.6,0,0,0,.5),
		float3x3(0,.8,0, .6,0,0, 0,0,.3)
	};

	SAM=0.0;
	float3x3 f=c[0];
	A(12.0) A(5.0) f=lerp(f,c[1],T);
	A(5.0)  A(5.0) f=lerp(f,c[0],T);
	A(6.0)  A(5.0) f=lerp(f,c[2],T);
	A(5.0)  A(5.0) f=lerp(f,c[0],T);
	A(6.0)  A(5.0) f=lerp(f,c[3],T);
	A(5.0)  A(5.0) f=lerp(f,c[0],T);

	float3 d=E(b);
	float3 eye=float3(.7,time*.01,time)+k;
	seq[0] = lookat(d);
	seq[1][0] = eye;
	seq[2] = f;

	float3 p=eye+d*.6;

	for(int i= 0;i<30;i++)
	{
		seq[i+10][0] =p+randCurve(time*.3+10.*hash[i].y,float(i))*.8;
	}
}
        
    )";

