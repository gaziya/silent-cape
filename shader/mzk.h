
// -------------------------------------------
//
//  +++++++++ MUSIC SHADER ++++++++++++++++++
//
// -------------------------------------
    

const char* mzkShaderSource =  R"(

RWStructuredBuffer<float2> gain: register(u0);

#define S(f,t) (lerp(float(f(int(t))&255),float(f(int(t+1.0))&255),smoothstep(0.0,1.0,frac(t)))/128.0-1.0)

int F(int t){return (t&t/255)-(t*4&t>>10&t>>7);}

float2 mainSound( float t)
{
	float g=0.;
    g+=.2*S(F,t*2000);
    g+=.2*S(F,(t-.01)*2000);
	return clamp((float2)g,-1.,1.);
}


[numthreads(32, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    float time = float(id) / 48000.0;
    gain[id] = mainSound(time)*.7;
}

)";

