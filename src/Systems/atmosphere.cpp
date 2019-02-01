#include "atmosphere.h"
#include "rf/context.h"
#include "rf/utils.h"
#include "Game/sun.h"
#include "rf/color.h"

#define USE_MOON 0
#define PRECOMPUTE_STUFF

namespace atmosphere2
{
	// NOTES
	// - the star(sun) is so far away its considered as directional (L direction)
	// - the planet is perfectly spherical (no terrain morphology)
	// - atmosphere density changes in respect with altitude but not longitude nor latitude (height based heterogeneity)
	// - Po is observer point
	// - 4 scalar parameters : 
	//		- h     in [0,Hmax] : height
	//		- theta in [0,pi]	: view-zenith angle
	//		- delta in [0,pi]	: sun-zenith angle
	////		- phi   in [0,pi]	: sun-view angle

	// All Lengths are in meters
	static const real32 Hmin = 6360000.f;
	static const real32 Hmax = 6420000.f;
	static const real32 Hr = 8000.0f; // atmospheric Rayleigh scale height (altitude where the density scales down by a 1/e term)
	static const real32 Hm = 1200.0f; // atmospheric Mie scale height

	static const vec3i  ScatteringTextureSize = vec3i(32, 256, 32);		// height, theta, delta
	static const vec2i  TextureTransmittanceSize = vec2i(512, 512);		// height, theta
	static const vec2i	PhaseTextureSize = vec2i(4096, 32);
	static const vec3i  WaterScatteringTextureSize = vec3i(32, 64, 64);
	static const int    AmbientTextureSize = 256;

	struct atmosphere_parameters
	{
		real32 Rp;	// planet radius
		real32 Ra;	// atmosphere radius
	};

	atmosphere_parameters Atmosphere;

	rf::mesh ScreenQuad = {};

	uint32 FmTexture = 0;
	uint32 TransmittanceTexture = 0;


	real32 RayleighDensityScale(real32 Height)
	{
		static const real32 invHr = 1.0f / Hr;
		return expf(-Height * invHr);
	}

	real32 MieDensityScale(real32 Height)
	{
		static const real32 invHm = 1.0f / Hm;
		return expf(-Height * invHm);
	}

	// Phase evaluation for a scattering angle theta
	real32 RayleighPhase(real32 Theta)
	{
		return 0.75f * (1.0f + pow(cos(Theta), 2));
	}

	// Henyey-Greenstein approximation to Mie Phase Function. g \In ]-1,1[
	real32 MiePhase(real32 Theta, real32 G)
	{
		real32 Gsq = G * G;
		real32 CosTheta = cos(Theta);
		real32 threehalf = 3.0f / 2.0f;
		return threehalf * ((1.0f - Gsq) / ((2.0f + Gsq))) * (1.0f + pow(CosTheta, 2)) / pow(1.0f + Gsq - 2.0f * G * CosTheta, threehalf);
	}

	void SetupAtmosphereShader(uint32 program)
	{
		rf::SendFloat(glGetUniformLocation(program, "Atmosphere.Rp"), Atmosphere.Rp);
		rf::SendFloat(glGetUniformLocation(program, "Atmosphere.Ra"), Atmosphere.Ra);
	}

	void Init(game::state *State, rf::context *Context)
	{
		// Atmosphere setup
		Atmosphere.Rp = Hmin;
		Atmosphere.Ra = Hmax;

		// - sampled at 15 evenly spaced wavelengths
		int const nWavelengths = (LAMBDA_MAX - LAMBDA_MIN) / 15;
		real32 Wavelengths[nWavelengths];
		real32 SolarIrradianceWavelengths[nWavelengths];
		real32 GroundAlbedoWavelengths[nWavelengths];
		real32 RayleighScatteringWavelengths[nWavelengths];
		real32 MieScatteringWavelengths[nWavelengths];
		real32 MieExtinctionWavelengths[nWavelengths];
		real32 AbsorptionExtinctionWavelengths[nWavelengths];
#if 0
		for (int l = LAMBDA_MIN; l <= LAMBDA_MAX; l += 10)
		{
			int Idx = (l - LAMBDA_MIN) / 15;
			real32 Lambda = (real32)l * 1e-3f; // micrometers
			real32 Mie = kMieAngstromBeta / kMieScaleHeight * std::pow(Lambda, -kMieAngstromAlpha);
			Wavelengths[Idx] = (real32)l;
			RayleighScatteringWavelengths[Idx] = kRayleigh * std::pow(Lambda, -4);
			MieScatteringWavelengths[Idx] = Mie * kMieSingleScatteringAlbedo;
			MieExtinctionWavelengths[Idx] = Mie;
			AbsorptionExtinctionWavelengths[Idx] = kMaxOzoneNumberDensity * kOzoneCrossSection[Idx];
			SolarIrradianceWavelengths[Idx] = USE_MOON ? kLunarIrradiance[Idx] * 1e-3f : kSolarIrradiance[Idx];
			GroundAlbedoWavelengths[Idx] = kGroundAlbedo;
		}
#endif

		path VSPath, FSPath, GSPath;

		rf::frame_buffer RenderBuffer = {};
		uint32 AtmospherePrecomputeProgram = 0;
		ScreenQuad = rf::Make2DQuad(Context, vec2i(-1, 1), vec2i(1, -1));

		rf::ConcatStrings(VSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_precompute_vert.glsl");
		rf::ConcatStrings(FSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_precompute_frag.glsl");
		AtmospherePrecomputeProgram = rf::BuildShader(Context, VSPath, FSPath);
		glUseProgram(AtmospherePrecomputeProgram);
		rf::CheckGLError("Atmosphere Precompute Shader");

		// Setup shader atmosphere uniforms
		SetupAtmosphereShader(AtmospherePrecomputeProgram);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex0"), 0);
		//rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex1"), 1);
		//rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex2"), 2);
		//rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex3"), 3);
		//rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex4"), 4);

		// Transmittance texture
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "ProgramUnit"), 0);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "TextureTransmittanceWidth"), TextureTransmittanceSize.x);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "TextureTransmittanceHeight"), TextureTransmittanceSize.y);
		rf::DestroyFramebuffer(&RenderBuffer);
		RenderBuffer = rf::MakeFramebuffer(1, TextureTransmittanceSize);
		glDeleteTextures(1, &TransmittanceTexture);
		TransmittanceTexture = rf::Make2DTexture(NULL, TextureTransmittanceSize.x, TextureTransmittanceSize.y, 4, true, false, 1,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		//FramebufferAttachBuffer(&RenderBuffer, 0, 4, true, false, false); // RGBA32F
		glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TransmittanceTexture, 0);
		glViewport(0, 0, TextureTransmittanceSize.x, TextureTransmittanceSize.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(ScreenQuad.VAO);
		rf::RenderMesh(&ScreenQuad);




		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
		glViewport(0, 0, Context->WindowWidth, Context->WindowHeight);
		glDeleteProgram(AtmospherePrecomputeProgram);
		rf::DestroyFramebuffer(&RenderBuffer);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnablei(GL_BLEND, 0);
		glEnablei(GL_BLEND, 1);
	}

	void Render(game::state *State, rf::context *Context)
	{
	}

	void ReloadShaders(rf::context *Context)
	{

	}
}


namespace atmosphere
{
    // Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
    // (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
    // Values are in W.m-2.nm-1
    static const real32 kSolarIrradiance[48] = {
        1.11776f, 1.14259f, 1.01249f, 1.14716f, 1.72765f, 1.73054f, 1.6887f, 1.61253f,
        1.91198f, 2.03474f, 2.02042f, 2.02212f, 1.93377f, 1.95809f, 1.91686f, 1.8298f,
        1.8685f, 1.8931f, 1.85149f, 1.8504f, 1.8341f, 1.8345f, 1.8147f, 1.78158f, 1.7533f,
        1.6965f, 1.68194f, 1.64654f, 1.6048f, 1.52143f, 1.55622f, 1.5113f, 1.474f, 1.4482f,
        1.41018f, 1.36775f, 1.34188f, 1.31429f, 1.28303f, 1.26758f, 1.2367f, 1.2082f,
        1.18737f, 1.14683f, 1.12362f, 1.1058f, 1.07124f, 1.04992f
    };

    // Values from "Precise Measurement of Lunar Spectral Irradiance at Visible Wavelengths"
    // (see https://www.researchgate.net/publication/272672351_Precise_Measurement_of_Lunar_Spectral_Irradiance_at_Visible_Wavelengths)
    // Values are in microW.m-2.nm-1
    static const real32 kLunarIrradiance[48] = {
        0.1916f, 0.4312f, 0.6708f, 0.9104f, 1.15f, 1.3896f, 1.6292f, 1.8688f,
        2.1084f, 2.348f, 2.3574f, 2.3668f, 2.3762f, 2.3856f, 2.395f, 2.4426f,
        2.4902f, 2.5378f, 2.5854f, 2.633f, 2.6402f, 2.6474f, 2.6546f, 2.6618f, 2.669f,
        2.6548f, 2.6406f, 2.6264f, 2.6122f, 2.598f, 2.5732f, 2.5484f, 2.5236f,
        2.4988f, 2.474f, 2.442f, 2.41f, 2.378f, 2.346f, 2.314f, 2.2696f,
        2.2252f, 2.1808f, 2.1364f, 2.092f, 2.0476f, 2.0032f // 0, 0, 1.870
    };

    static const real32 kOzoneCrossSection[48] = {
        1.18e-27f, 2.182e-28f, 2.818e-28f, 6.636e-28f, 1.527e-27f, 2.763e-27f, 5.52e-27f,
        8.451e-27f, 1.582e-26f, 2.316e-26f, 3.669e-26f, 4.924e-26f, 7.752e-26f, 9.016e-26f,
        1.48e-25f, 1.602e-25f, 2.139e-25f, 2.755e-25f, 3.091e-25f, 3.5e-25f, 4.266e-25f,
        4.672e-25f, 4.398e-25f, 4.701e-25f, 5.019e-25f, 4.305e-25f, 3.74e-25f, 3.215e-25f,
        2.662e-25f, 2.238e-25f, 1.852e-25f, 1.473e-25f, 1.209e-25f, 9.423e-26f, 7.455e-26f,
        6.566e-26f, 5.105e-26f, 4.15e-26f, 4.228e-26f, 3.237e-26f, 2.451e-26f, 2.801e-26f,
        2.534e-26f, 1.624e-26f, 1.465e-26f, 2.078e-26f, 1.383e-26f, 7.105e-27f
    };



    /// Defined as "ExpTerm * exp(ExpScale * H) + LinearTerm * H + ConstantTerm"
    /// Clamped in [0,1]
    struct density_profile_layer
    {
        real32 Width;
        real32 ExpTerm;
        real32 ExpScale;
        real32 LinearTerm;
        real32 ConstantTerm;
    };

    /// Atmosphere is made of several layers from bottom to top
    /// The height of the topmost layer is inconsequential as it extends always to the top of the atmosphere
    struct density_profile
    {
        density_profile_layer Layers[2];
    };
    
    struct atmosphere_parameters
    {
        real32 TopRadius;           // distance between planet's center and top of atmosphere
        real32 BottomRadius;        // distance between planet's center and bottom of atmosphere
        vec3f  RayleighScattering;  // scattering coefficient of air molecules at max density (function of wavelength)
        density_profile Rayleigh;   // density profile of air molecules
        vec3f  MieScattering;       // scattering coefficient of aerosols at max density (function of wavelength)
        vec3f  MieExtinction;       // extinction coefficient of aerosols at max density (function of wavelength)
        density_profile Mie;        // density profile of aerosols
        //vec3f  AbsorptionScattering;// scattering coefficient of air molecules that absorb light at max density (function of wavelength)
        vec3f  AbsorptionExtinction;// extinction coefficient of air molecules that absorb light at max density (function of wavelength);
        density_profile Absorption; // density profile of air molecules that absorb light (e.g. ozone)
        vec3f  GroundAlbedo;        // average albedo of the ground
        vec3f  SolarIrradiance;     // defined at the top of the atmosphere
        real32 SunAngularRadius;    // < 0.1 radians
        real32 MiePhaseG;           // asymmetry coefficient for the Mie phase function
        real32 MinMuS;              // Cosine of the maximum sun zenith (102deg for earth, so that MinMuS = -0.208)

    };

	radiance_mode RadianceMode = RGB;
	int NumPrecomputedWavelengths = RadianceMode == FULL ? 15 : 3;
	real32 DefaultWavelengths[] = { LAMBDA_R, LAMBDA_G, LAMBDA_B };

    atmosphere_parameters AtmosphereParameters;
    rf::mesh ScreenQuad = {};
    uint32 AtmosphereProgram = 0;
    uint32 TransmittanceTexture = 0;
    uint32 IrradianceTexture = 0;
    uint32 ScatteringTexture = 0;
    uint32 MoonAlbedoTexture = 0;

	vec3f WhitePoint(1.0f);

    static const vec2i  kTransmittanceTextureSize = vec2i(256, 64);
    static const vec2i  kIrradianceTextureSize = vec2i(64, 16);
    static const vec4i  kScatteringTextureRMuMuSNuSize = vec4i(64, 128, 32, 8);
    static const vec3i  kScatteringTextureSize = vec3i(kScatteringTextureRMuMuSNuSize.w * kScatteringTextureRMuMuSNuSize.z, kScatteringTextureRMuMuSNuSize.y, kScatteringTextureRMuMuSNuSize.x);

    static const real32 kLengthUnitInMeters = 1000.f;
    static const real32 kRayleighScaleHeight = 8000.f;
	static const real32 kRayleigh = 1.24062e-6f;
    static const real32 kMieScaleHeight = 1200.f;
    static const real32 kMieAngstromAlpha = 0.f;
	static const real32 kMieAngstromBeta = 5.328e-3f;
    static const real32 kMieSingleScatteringAlbedo = 0.9f;
    static const real32 kDobsonUnit = 2.687e20f; // From wiki, in molecules.m^-2
    static const real32 kMaxOzoneNumberDensity = 300.f * kDobsonUnit / 15000.f; // Max nb density of ozone molecules in m^-3, 300 DU integrated over the ozone density profile (15km)
    static const real32 kGroundAlbedo = 0.1f; // orig 0.1
    static const real32 kSunAngularRadius = 0.004675f;
    static const real32 kMoonAngularRadius = 0.018f;//0.004509f;
    static const real32 kSunSolidAngle = M_PI * kSunAngularRadius * kSunAngularRadius;
    static const real32 kMiePhaseG = USE_MOON ? 0.93f : 0.80f;
    static const real32 kMaxSunZenithAngle = DEG2RAD * 120.f;
	
    static void SendShaderUniforms(uint32 Program)
    {
        glUseProgram(Program);
        // TODO - Write the constants directly in the header (construct it from here), instead of this disgusting mess
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.BottomRadius"), AtmosphereParameters.BottomRadius / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.TopRadius"), AtmosphereParameters.TopRadius / kLengthUnitInMeters);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.RayleighScattering"), AtmosphereParameters.RayleighScattering);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[0].Width"), AtmosphereParameters.Rayleigh.Layers[0].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[0].ExpTerm"), AtmosphereParameters.Rayleigh.Layers[0].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[0].ExpScale"), AtmosphereParameters.Rayleigh.Layers[0].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[0].LinearTerm"), AtmosphereParameters.Rayleigh.Layers[0].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[0].ConstantTerm"), AtmosphereParameters.Rayleigh.Layers[0].ConstantTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[1].Width"), AtmosphereParameters.Rayleigh.Layers[1].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[1].ExpTerm"), AtmosphereParameters.Rayleigh.Layers[1].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[1].ExpScale"), AtmosphereParameters.Rayleigh.Layers[1].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[1].LinearTerm"), AtmosphereParameters.Rayleigh.Layers[1].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.RayleighDensity.Layers[1].ConstantTerm"), AtmosphereParameters.Rayleigh.Layers[1].ConstantTerm);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.MieExtinction"), AtmosphereParameters.MieExtinction);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.MieScattering"), AtmosphereParameters.MieScattering);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[0].Width"), AtmosphereParameters.Mie.Layers[0].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[0].ExpTerm"), AtmosphereParameters.Mie.Layers[0].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[0].ExpScale"), AtmosphereParameters.Mie.Layers[0].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[0].LinearTerm"), AtmosphereParameters.Mie.Layers[0].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[0].ConstantTerm"), AtmosphereParameters.Mie.Layers[0].ConstantTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[1].Width"), AtmosphereParameters.Mie.Layers[1].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[1].ExpTerm"), AtmosphereParameters.Mie.Layers[1].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[1].ExpScale"), AtmosphereParameters.Mie.Layers[1].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[1].LinearTerm"), AtmosphereParameters.Mie.Layers[1].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MieDensity.Layers[1].ConstantTerm"), AtmosphereParameters.Mie.Layers[1].ConstantTerm);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.AbsorptionExtinction"), AtmosphereParameters.AbsorptionExtinction);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[0].Width"), AtmosphereParameters.Absorption.Layers[0].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[0].ExpTerm"), AtmosphereParameters.Absorption.Layers[0].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[0].ExpScale"), AtmosphereParameters.Absorption.Layers[0].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[0].LinearTerm"), AtmosphereParameters.Absorption.Layers[0].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[0].ConstantTerm"), AtmosphereParameters.Absorption.Layers[0].ConstantTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[1].Width"), AtmosphereParameters.Absorption.Layers[1].Width / kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[1].ExpTerm"), AtmosphereParameters.Absorption.Layers[1].ExpTerm);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[1].ExpScale"), AtmosphereParameters.Absorption.Layers[1].ExpScale * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[1].LinearTerm"), AtmosphereParameters.Absorption.Layers[1].LinearTerm * kLengthUnitInMeters);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.AbsorptionDensity.Layers[1].ConstantTerm"), AtmosphereParameters.Absorption.Layers[1].ConstantTerm);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.GroundAlbedo"), AtmosphereParameters.GroundAlbedo);
        rf::SendVec3(glGetUniformLocation(Program, "Atmosphere.SolarIrradiance"), AtmosphereParameters.SolarIrradiance);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.SunAngularRadius"), AtmosphereParameters.SunAngularRadius);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MiePhaseG"), AtmosphereParameters.MiePhaseG);
        rf::SendFloat(glGetUniformLocation(Program, "Atmosphere.MinMuS"), AtmosphereParameters.MinMuS);
        rf::CheckGLError("Atmosphere Uniform Shader");
    }

	void InitializeModel(rf::context *Context)
	{
		path VSPath, FSPath, GSPath;

		rf::ConcatStrings(VSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_precompute_vert.glsl");
		rf::ConcatStrings(FSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_precompute_frag.glsl");
		uint32 AtmospherePrecomputeProgram = rf::BuildShader(Context, VSPath, FSPath);
		rf::ConcatStrings(GSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_precompute_geom.glsl");
		uint32 ScatteringProgram = rf::BuildShader(Context, VSPath, FSPath, GSPath);


		// Precompute atmosphere textures
		rf::frame_buffer RenderBuffer = {};

		glUseProgram(AtmospherePrecomputeProgram);
		rf::CheckGLError("Atmosphere Precompute Shader");

		SendShaderUniforms(AtmospherePrecomputeProgram);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex0"), 0);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex1"), 1);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex2"), 2);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex3"), 3);
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "Tex4"), 4);

		// Transmittance texture
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "ProgramUnit"), 0);
		rf::DestroyFramebuffer(&RenderBuffer);
		RenderBuffer = rf::MakeFramebuffer(1, kTransmittanceTextureSize);
		glDeleteTextures(1, &TransmittanceTexture);
		TransmittanceTexture = rf::Make2DTexture(NULL, kTransmittanceTextureSize.x, kTransmittanceTextureSize.y, 4, true, false, 1,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		//FramebufferAttachBuffer(&RenderBuffer, 0, 4, true, false, false); // RGBA32F
		glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TransmittanceTexture, 0);
		glViewport(0, 0, kTransmittanceTextureSize.x, kTransmittanceTextureSize.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(ScreenQuad.VAO);
		rf::RenderMesh(&ScreenQuad);

		// Ground Irradiance texture
		rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "ProgramUnit"), 1);
		rf::DestroyFramebuffer(&RenderBuffer);
		RenderBuffer = rf::MakeFramebuffer(2, kIrradianceTextureSize);
		glDeleteTextures(1, &IrradianceTexture);
		IrradianceTexture = rf::Make2DTexture(NULL, kIrradianceTextureSize.x, kIrradianceTextureSize.y, 4, true, false, 1,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		uint32 DeltaIrradianceTexture = rf::Make2DTexture(NULL, kIrradianceTextureSize.x, kIrradianceTextureSize.y, 4, true, false, 1,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, DeltaIrradianceTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, IrradianceTexture, 0);
		glViewport(0, 0, kIrradianceTextureSize.x, kIrradianceTextureSize.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TransmittanceTexture);
		glBindVertexArray(ScreenQuad.VAO);
		rf::RenderMesh(&ScreenQuad);

		// Single Scattering
		glUseProgram(ScatteringProgram);
		SendShaderUniforms(ScatteringProgram);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "ProgramUnit"), 2);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "Tex0"), 0);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "Tex1"), 1);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "Tex2"), 2);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "Tex3"), 3);
		rf::SendInt(glGetUniformLocation(ScatteringProgram, "Tex4"), 4);
		rf::CheckGLError("Scattering Precompute Shader");

		rf::DestroyFramebuffer(&RenderBuffer);
		RenderBuffer = rf::MakeFramebuffer(3, vec2i(kScatteringTextureSize.x, kScatteringTextureSize.y), false);
		glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
		glDeleteTextures(1, &ScatteringTexture);
		ScatteringTexture = rf::Make3DTexture(kScatteringTextureSize.x, kScatteringTextureSize.y, kScatteringTextureSize.z, 4, true, false,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		uint32 DeltaRayleighTexture = rf::Make3DTexture(kScatteringTextureSize.x, kScatteringTextureSize.y, kScatteringTextureSize.z, 4, true, false,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		uint32 DeltaMieTexture = rf::Make3DTexture(kScatteringTextureSize.x, kScatteringTextureSize.y, kScatteringTextureSize.z, 4, true, false,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		rf::FramebufferAttachBuffer(&RenderBuffer, 0, DeltaRayleighTexture);
		rf::FramebufferAttachBuffer(&RenderBuffer, 1, DeltaMieTexture);
		rf::FramebufferAttachBuffer(&RenderBuffer, 2, ScatteringTexture);
		D(if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			LogError("Incomplete Scattering Framebuffer");
		})
			glViewport(0, 0, kScatteringTextureSize.x, kScatteringTextureSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TransmittanceTexture);
		glBindVertexArray(ScreenQuad.VAO);
		uint32 LayerLoc = glGetUniformLocation(ScatteringProgram, "ScatteringLayer");
		for (int layer = 0; layer < kScatteringTextureSize.z; ++layer)
		{
			rf::SendInt(LayerLoc, layer);
			rf::RenderMesh(&ScreenQuad);
		}

		rf::CheckGLError("Scattering Precomputation");

		// Multiple Scattering
		uint32 DeltaScatteringDensityTexture = rf::Make3DTexture(kScatteringTextureSize.x, kScatteringTextureSize.y, kScatteringTextureSize.z, 4, true, false,
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		glDisablei(GL_BLEND, 0);
		glDisablei(GL_BLEND, 1);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		uint32 NumScatteringBounces = 4;
		for (uint32 bounce = 2; bounce <= NumScatteringBounces; ++bounce)
		{
			// 1. Compute Scattering density into DeltaScatteringDensityTexture
			glUseProgram(ScatteringProgram);
			rf::SendInt(glGetUniformLocation(ScatteringProgram, "ProgramUnit"), 3);
			rf::FramebufferSetAttachmentCount(&RenderBuffer, 1);
			rf::FramebufferAttachBuffer(&RenderBuffer, 0, DeltaScatteringDensityTexture);
			rf::FramebufferAttachBuffer(&RenderBuffer, 1, 0);
			rf::FramebufferAttachBuffer(&RenderBuffer, 2, 0);
			rf::FramebufferAttachBuffer(&RenderBuffer, 3, 0);
			rf::CheckFramebufferError("Scattering Density Framebuffer");
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TransmittanceTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_3D, DeltaRayleighTexture);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_3D, DeltaMieTexture);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_3D, DeltaRayleighTexture);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, DeltaIrradianceTexture);
			rf::SendInt(glGetUniformLocation(ScatteringProgram, "ScatteringBounce"), bounce);
			for (int32 layer = 0; layer < kScatteringTextureSize.z; ++layer)
			{
				rf::SendInt(LayerLoc, layer);
				rf::RenderMesh(&ScreenQuad);
			}

			// 2. Compute Indirect irradiance into DeltaIrradianceTexture, accumulate it into IrradianceTexture
			glUseProgram(AtmospherePrecomputeProgram);
			rf::SendInt(glGetUniformLocation(ScatteringProgram, "ProgramUnit"), 4);
			rf::FramebufferSetAttachmentCount(&RenderBuffer, 2);
			rf::FramebufferAttachBuffer(&RenderBuffer, 0, DeltaIrradianceTexture);
			rf::FramebufferAttachBuffer(&RenderBuffer, 1, IrradianceTexture);
			rf::CheckFramebufferError("Indirect Irradiance Framebuffer");
			rf::BindTexture3D(DeltaRayleighTexture, 1);
			rf::BindTexture3D(DeltaMieTexture, 2);
			rf::BindTexture3D(DeltaRayleighTexture, 3);
			rf::SendInt(glGetUniformLocation(AtmospherePrecomputeProgram, "ScatteringBounce"), bounce - 1);
			// Accumulate Irradiance in Attachment 1
			glEnablei(GL_BLEND, 1);
			rf::RenderMesh(&ScreenQuad);
			glDisablei(GL_BLEND, 1);

			// 3. Compute Multiple scattering into RayleighTexture
			glUseProgram(ScatteringProgram);
			rf::SendInt(glGetUniformLocation(ScatteringProgram, "ProgramUnit"), 5);
			rf::FramebufferSetAttachmentCount(&RenderBuffer, 2);
			rf::FramebufferAttachBuffer(&RenderBuffer, 0, DeltaRayleighTexture);
			rf::FramebufferAttachBuffer(&RenderBuffer, 1, ScatteringTexture);
			rf::CheckFramebufferError("MS Framebuffer");
			rf::BindTexture2D(TransmittanceTexture, 0);
			rf::BindTexture3D(DeltaScatteringDensityTexture, 1);
			glEnablei(GL_BLEND, 1);
			for (int32 layer = 0; layer < kScatteringTextureSize.z; ++layer)
			{
				rf::SendInt(LayerLoc, layer);
				rf::RenderMesh(&ScreenQuad);
			}
			glDisablei(GL_BLEND, 1);
		}


		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
		glViewport(0, 0, Context->WindowWidth, Context->WindowHeight);

		glDeleteTextures(1, &DeltaIrradianceTexture);
		glDeleteTextures(1, &DeltaRayleighTexture);
		glDeleteTextures(1, &DeltaMieTexture);
		glDeleteTextures(1, &DeltaScatteringDensityTexture);
		glDeleteProgram(ScatteringProgram);
		glDeleteProgram(AtmospherePrecomputeProgram);
		rf::DestroyFramebuffer(&RenderBuffer);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnablei(GL_BLEND, 0);
		glEnablei(GL_BLEND, 1);

		//MoonAlbedoTexture = *rf::ResourceLoad2DTexture(Context, "data/moon/albedo.png", false, false, 4, 
				//GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_CLAMP_TO_EDGE);
	}

    void Init(game::state *State, rf::context *Context)
    {
		(void) State;
        ScreenQuad = rf::Make2DQuad(Context, vec2i(-1,1), vec2i(1, -1));

        AtmosphereParameters.TopRadius = 6420000.f;
        AtmosphereParameters.BottomRadius = 6360000.f;

        density_profile_layer DefaultLayer = { 0.f, 0.f, 0.f, 0.f, 0.f };
        density_profile_layer RayleighLayer = { 0.f, 1.f, -1.f / kRayleighScaleHeight, 0.f, 0.f };
        density_profile_layer MieLayer = { 0.f, 1.f, -1.f / kMieScaleHeight, 0.f, 0.f };
        density_profile_layer Ozone0Layer = { 25000.f, 0.f, 0.f, 1.f / 15000.f, -2.f / 3.f };
        density_profile_layer Ozone1Layer = { 0.f, 0.f, 0.f, -1.f / 15000.f, 8.f / 3.f };

        // Compute absorption and scattering SRGB colors from wavelength
        int const nWavelengths = (LAMBDA_MAX-LAMBDA_MIN) / 10;
		real32 *Wavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *SolarIrradianceWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *GroundAlbedoWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *RayleighScatteringWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *MieScatteringWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *MieExtinctionWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));
		real32 *AbsorptionExtinctionWavelengths = (real32*)PushArenaData(Context->ScratchArena, nWavelengths * sizeof(real32));

        for(int l = LAMBDA_MIN; l <= LAMBDA_MAX; l += 10)
        {
            int Idx = (l-LAMBDA_MIN)/10;
            real32 Lambda = (real32)l * 1e-3f; // micrometers
            real32 Mie = kMieAngstromBeta / kMieScaleHeight * std::pow(Lambda, -kMieAngstromAlpha);
            Wavelengths[Idx] = (real32)l;
            RayleighScatteringWavelengths[Idx] = kRayleigh * std::pow(Lambda, -4);
            MieScatteringWavelengths[Idx] = Mie * kMieSingleScatteringAlbedo;
            MieExtinctionWavelengths[Idx] = Mie;
            AbsorptionExtinctionWavelengths[Idx] = kMaxOzoneNumberDensity * kOzoneCrossSection[Idx];
            SolarIrradianceWavelengths[Idx] = USE_MOON ? kLunarIrradiance[Idx] * 1e-3f : kSolarIrradiance[Idx];
            GroundAlbedoWavelengths[Idx] = kGroundAlbedo;
        }

		// Extract from above data depending on radiance rendering mode
		int nW = 3;

		AtmosphereParameters.RayleighScattering = ConvertSpectrumToSRGB(Wavelengths, RayleighScatteringWavelengths, nWavelengths, kLengthUnitInMeters);
        AtmosphereParameters.Rayleigh.Layers[0] = DefaultLayer;
        AtmosphereParameters.Rayleigh.Layers[1] = RayleighLayer;
        AtmosphereParameters.MieExtinction = ConvertSpectrumToSRGB(Wavelengths, MieExtinctionWavelengths, nWavelengths, kLengthUnitInMeters);
        AtmosphereParameters.MieScattering = ConvertSpectrumToSRGB(Wavelengths, MieScatteringWavelengths, nWavelengths, kLengthUnitInMeters);
        AtmosphereParameters.Mie.Layers[0] = DefaultLayer;
        AtmosphereParameters.Mie.Layers[1] = MieLayer;
        AtmosphereParameters.AbsorptionExtinction = ConvertSpectrumToSRGB(Wavelengths, AbsorptionExtinctionWavelengths, nWavelengths, kLengthUnitInMeters);
        AtmosphereParameters.Absorption.Layers[0] = Ozone0Layer;
        AtmosphereParameters.Absorption.Layers[1] = Ozone1Layer;
        AtmosphereParameters.GroundAlbedo = ConvertSpectrumToSRGB(Wavelengths, GroundAlbedoWavelengths, nWavelengths, 1.0);
        AtmosphereParameters.SolarIrradiance = ConvertSpectrumToSRGB(Wavelengths, SolarIrradianceWavelengths, nWavelengths, 1.0);
        AtmosphereParameters.SunAngularRadius = USE_MOON ? kMoonAngularRadius : kSunAngularRadius;
        AtmosphereParameters.MiePhaseG = kMiePhaseG;
        AtmosphereParameters.MinMuS = cosf(kMaxSunZenithAngle);

#ifdef PRECOMPUTE_STUFF
		InitializeModel(Context);
#endif
    }

    void Update()
    {
    }

    void Render(game::state *State, rf::context *Context)
    {
        mat4f ViewMatrix = State->Camera.ViewMatrix;

        real32 AspectRatio = Context->WindowWidth / (real32)Context->WindowHeight;
        real32 FovRadians = HFOVtoVFOV(AspectRatio, Context->FOV) * DEG2RAD;
        real32 TanFovY = tanf(FovRadians * 0.5f);
        mat4f ProjMatrix
        (
            1.f/(TanFovY * AspectRatio), 0.f, 0.f, 0.f,
            0.f, 1.f/TanFovY, 0.f, 0.f,
            0.f, 0.f, 1.f, 1.f,
            0.f, 0.f, -1.f, 0.f
        );

        glDepthFunc(GL_LEQUAL);

        //vec3f Camera;
        real32 CameraScale = 1.0f / (kLengthUnitInMeters);
        //Camera.x = ModelFromView.M[3].x * l;
        //Camera.y = ModelFromView.M[3].y * l;
        //Camera.z = ModelFromView.M[3].z * l;
    

        glUseProgram(AtmosphereProgram);
        rf::SendMat4(glGetUniformLocation(AtmosphereProgram, "ViewMatrix"), ViewMatrix);
        rf::SendMat4(glGetUniformLocation(AtmosphereProgram, "ProjMatrix"), ProjMatrix);
        rf::SendFloat(glGetUniformLocation(AtmosphereProgram, "CameraScale"), CameraScale);
        rf::SendVec3(glGetUniformLocation(AtmosphereProgram, "SunDirection"), State->SunDirection);
        rf::SendFloat(glGetUniformLocation(AtmosphereProgram, "Time"), (real32)State->EngineTime);
        rf::SendVec2(glGetUniformLocation(AtmosphereProgram, "Resolution"), vec2f((real32)Context->WindowWidth, (real32)Context->WindowHeight));
        rf::CheckGLError("Atmo0");
#ifdef PRECOMPUTE_STUFF
        rf::BindTexture2D(TransmittanceTexture, 0);
        rf::BindTexture2D(IrradianceTexture, 1);
        rf::BindTexture3D(ScatteringTexture, 2);
#else
        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 0);
        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 1);
        //BindTexture3D(*Context->RenderResources.DefaultDiffuseTexture, 2);
#endif
        rf::CheckGLError("Atmo1");
        rf::BindTexture2D(USE_MOON ? MoonAlbedoTexture : *Context->RenderResources.DefaultDiffuseTexture, 3);
        glBindVertexArray(ScreenQuad.VAO);
        rf::RenderMesh(&ScreenQuad);
        glUseProgram(0);


        glDepthFunc(GL_LESS);
        rf::CheckGLError("Atmo");
    }

    void ReloadShaders(rf::context *Context)
    {
        path VSPath, FSPath;

        // Rendering shader
        rf::ConcatStrings(VSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_vert.glsl");
        rf::ConcatStrings(FSPath, rf::ctx::GetExePath(Context), "data/shaders/atmosphere_frag.glsl");
        AtmosphereProgram = rf::BuildShader(Context, VSPath, FSPath);
        glUseProgram(AtmosphereProgram);
        rf::CheckGLError("Atmosphere Shader");

        // Init constants
        SendShaderUniforms(AtmosphereProgram);
        rf::SendInt(glGetUniformLocation(AtmosphereProgram, "TransmittanceTexture"), 0);
        rf::SendInt(glGetUniformLocation(AtmosphereProgram, "IrradianceTexture"), 1);
        rf::SendInt(glGetUniformLocation(AtmosphereProgram, "ScatteringTexture"), 2);
        rf::SendInt(glGetUniformLocation(AtmosphereProgram, "MoonAlbedo"), 3);

        glUseProgram(0);
    }
}

