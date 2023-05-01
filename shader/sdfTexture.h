
const char* sSdfTexture = R"(

cbuffer _:register(b0)
{
	float2 resolution;
};
        
RWStructuredBuffer<float4> image : register(u2);
    
float2 bezier(float2 a, float2 b, float2 c, float t)
{
	t=clamp(t,0.0,1.0);
	return lerp(lerp(a,b,t),lerp(b,c,t),t);
}

float2 bezier(float2 a, float2 b, float2 c, float2 d, float t)
{
	return lerp(bezier(a,b,c,t),bezier(b,c,d,t),t);
}
    
float lpnorm(float2 p, float n)
{
	p = pow(abs(p), (float2)n);
	return pow(p.x+p.y, 1.0/n);
}
    

float deBezier3(float2 p, float2 a, float2 b, float2 c, float2 d)
{
	float ITR = 50.0, pitch = 1.0, t = 0.5, de = 1e10;   
	for(int j=0; j<2; j++)
	{
		float t0 = t-pitch*0.5;
		pitch /= ITR;
		for(float i=0.0; i<=ITR; i++)
		{
			float de0=lpnorm(p-bezier(a,b,c,d,t0), 3.);
			if (de0<de)
			{
				de = de0;
				t=t0;
			}
			t0 += pitch;
		}
	}
	return de;
}


float deLine3(float2 p, float2 a, float2 b) {
	float2 pa = p - a, ba = b - a;
	return lpnorm(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0), 3.);
}


	
[numthreads(32, 32, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
	uint ID = id.y * 2048u + id.x;
	uint2 area = id / 512u;
	float2 uv = float2(id % 512u)/(float2)512;
	uv = uv * 2. - 1.;
	float4 O;
	int No = dot(area, int2(16,4));
	for(int i=0; i<4; i++)
	{
		if(No == 0) // A
		{
			O[i] = min(min(min(
				deLine3(uv, float2(-.5,-.5), float2(.2,.5)) - .05,
				deLine3(uv, float2(.2,.5), float2(.4,.5)) - .05),
				deLine3(uv, float2(.4,.5), float2(.5,-.5)) - .05),
				deLine3(uv, float2(-.3,-.2), float2(.4,-.2)) - .05
			);
		}
		if(No == 2) // C
		{
			O[i] = deBezier3(uv, float2(.6,-.5), float2(-.9,-.9), float2(-.9,.9), float2(.6,.5)) - .05;
		}
		if(No == 4) // E
		{
			O[i] = min(min(min(
				deLine3(uv, float2(-.5,-.6), float2(-.5,.5)) - .05,
				deLine3(uv, float2(-.5,.5), float2(.4,.5)) - .05),
				deLine3(uv, float2(-.5,.0), float2(.2,.0)) - .05),
				deLine3(uv, float2(-.5,-.6), float2(.4,-.6)) - .05
			);
		}
		if(No == 6) // G
		{
			O[i] = min(min(
				deBezier3(uv,
					float2(.3,-.4),
					float2(-.9,-.8),
					float2(-.9,1.),
					float2(.6,.6)
				) - .05,
				deLine3(uv,
					float2(.45,.1),
					float2(.35,-.5)
				) - .05),
				deLine3(uv,
					float2(-.1,.1),
					float2(.6,.1)
				) - .05
			);
		}
		if(No == 8) // I
		{
			O[i] = min(min(
				deLine3(uv, float2(-.1,.5), float2(.1,.5)) - .05,
				deLine3(uv, float2(.0,.5), float2(.0,-.6)) - .05),
				deLine3(uv, float2(-.1,-.6), float2(.1,-.6)) - .05
			);
		}
		if(No == 11) // L
		{
			O[i] =min(
				deLine3(uv, float2(-.5,-.6), float2(-.5,.5)) - .05,
				deLine3(uv, float2(-.5,-.6), float2(.4,-.6)) - .05
			);
		}
		if(No == 13) // N
		{
			O[i] =  min(min(
				deLine3(uv, float2(-.5,.7), float2(-.5,-.7)) - .05,
				deLine3(uv, float2(-.5,.7), float2(.5,-.7)) - .05),
				deLine3(uv, float2(.5,.7), float2(.5,-.7)) - .05
			);
		}
		if(No == 15) // P
		{
			O[i] = min(
				deBezier3(uv, float2(-.55,-.3), float2(.9,-.7), float2(.9,1.1), float2(-.55,.7)) - .05,
				deLine3(uv,float2(-.6,-.8),float2(-.6,.8))-.05
			);
		}
		if(No == 18) // S
		{
			O[i] = min(
				deBezier3(uv,
					float2(.3,.6),
					float2(-.2,1.2),
					float2(-.9,.3),
					float2(-.1,.1)
				) - .05,
				deBezier3(uv,
					float2(-.1,.1),
					float2(.6,-.1),
					float2(.0,-1.1),
					float2(-.8,-.5)) - .05
			);
		}
		if(No == 19) // T
		{
			O[i] =min(
				deLine3(uv, float2(.0,-.7), float2(.0,.5)) - .05,
				deLine3(uv, float2(-.5,.5), float2(.4,.5)) - .05
			);
		}
		if(No == 25) // Z
		{
			O[i] =min(min(
				deLine3(uv, float2(-.4,.5), float2(.5,.5)) - .05,
				deLine3(uv, float2(.5,.5), float2(-.5,-.5)) - .05),
				deLine3(uv, float2(-.5,-.5), float2(.5,-.5)) - .05
			);
		}
		No++;
	}
	image[ID] = O;
}

)";
