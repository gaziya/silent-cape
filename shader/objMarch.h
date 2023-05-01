
const char* sObjMarch = R"(

cbuffer _:register(b0)
{
	float2 R;
	float time;
};
        
RWStructuredBuffer<float4> hash : register(u1);
RWStructuredBuffer<float3x3> seq : register(u3);
RWStructuredBuffer<float4> bb : register(u4);
RWStructuredBuffer<float4> im : register(u8);

#define U(a)asuint(a)%0x400000
#define H(h) (cos((h)*6.3+float3(0,23,21))*.5+.5)
#define sabs(x) sqrt(x*x+5e-6)
#define PI 3.14159265

float3 rot(float3 p,float3 a,float t)
{
	a=normalize(a);
	return lerp(a*dot(p,a),p,cos(t))+sin(t)*cross(p,a);
}

float3 foldVec(float t)
{
	float3 n=float3(-.5,-cos(PI/t),0);
	n.z=sqrt(1.-dot(n,n));
	return n;
}

float3 fold(float3 p, float t)
{
	float3 n=foldVec(t);
	for(float i=0.; i<t; i++)
	{
		p.xy=sabs(p.xy);
		float g=dot(p,n);
		p-=(g-sabs(g))*n;
	}
	return p;
}

float map(float3 p, int id)
{
	p=rot(p,H(abs(hash[id].y)+time*.2)*2-1.,time*.5);
	float t=4.+step(0.,hash[id].w);
	p = fold(p,t);
	p.z-=.07;
	p=rot(p,float3(0,0,1),-PI/2.*(sin(time+hash[id].x*100.)*.5+.5));
	float3 a=float3(.005,.1,.005);
	return length(p-clamp(p,-a,a))-.003;
}

float3 calcNormal(float3 p, int id)
{
	float3 n=(float3)0;
	for(int i=0; i<4; i++){
		float3 e=.001*(float3(9>>i&1, i>>1&1, i&1)*2.-1.);
		n+=e*map(p+e,id);
	}
	return normalize(n);
}

[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	if( any( id >= uint2(R) ) )return;
	float2 uv = (float2(id*2)-R)/R.y;

	float3 rd=mul(seq[0],normalize(float3(uv,2.)));
	float3 ro=seq[1][0];
	float b=bb[(id.y/2) * uint(R.x) + id.x/2].x;
	float3 X[2]={float3(.2,.5,.8),float3(.9,.3,.2)}; 
	float z=100.;
	float4 O = (float4)z;
	for(int j=0;j<30;j++)
	{
		float t=b,e;
		float3 a=seq[10+j][0];
		if(length(cross(rd,ro-a))<.1)
	    for(int i=0;i<30;i++)
    	{
    		float3 p=ro+rd*t;
        	t+=e=map(p-a,j);
    		if ( e<.001)
    		{
    			if(z>t)
    			{
    				z=t;
    				float3 c=X[int(hash[j].z*3)&1];
    				float3 n = calcNormal(p-a,j);      
    				float3 li = (float3).577;
    				c *= clamp(dot(n, li), 0.2, 1.0);
        			float rimd = pow(clamp(1.0 - dot(reflect(-li, n), -rd), 0.0, 1.0), 2.5);
					float frn = rimd+2.2*(1.-rimd);
    				c *= frn*0.5;
    				c *= max(.5+.5*n.y, 0.);
    				c *= exp2(-1.*pow(max(0.0, 1.0-map(p+n*0.3, id)/0.3),2.0));
    				c += float3(.9,.8,.7)*pow(clamp(dot(reflect(rd, n), li), 0.0, 1.0), 10.0);
    				c = lerp((float3).1,c, exp(-.08*t*t));
    				O=float4(sqrt(c),t);
    			}
    			break;
    		}
    	}
    }	
	im[id.y * uint(R.x) + id.x] = O;
}
    )";

