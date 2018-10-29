#include "planet.h"
#include "rf/context.h"
#include "rf/utils.h"
#include "Game/sun.h"

namespace planet
{
	struct planet_params
	{
		mat4f ModelMatrix;
		mat4f ViewMatrix;
		vec3f CameraPosition;
		float OuterTessFactor;
		vec3f TileSize;
		float InnerTessFactor;
		vec3f Origin;
		float Time;
		int GridW;
		int GridH;
		int GridD;
	};

	planet_params PlanetParams;
	uint32 TessTestShader;
	uint32 PlanetUBO;
	rf::mesh PlanetMesh;

	void Init(game::state * State, rf::context * Context)
	{
		// 1 vertex VBO
		float vtxData[] = { 0.0f, 0.0f, 0.0f };
		uint32 idxData = 0;
		PlanetMesh.IndexCount = 1;
		PlanetMesh.IndexType = GL_UNSIGNED_INT;
		PlanetMesh.VAO = rf::MakeVertexArrayObject();
		PlanetMesh.VBO[0] = rf::AddIBO(GL_STATIC_DRAW, sizeof(uint32), &idxData);
		PlanetMesh.VBO[1] = rf::AddEmptyVBO(sizeof(vtxData), GL_STATIC_DRAW);
		rf::FillVBO(0, 3, GL_FLOAT, 0, sizeof(vtxData), vtxData);
		glBindVertexArray(0);
		rf::CheckGLError("Init");
		PlanetUBO = rf::MakeUBO(sizeof(planet_params), GL_STREAM_DRAW);
		rf::CheckGLError("InitUBO");
	}

	void Destroy()
	{
		rf::DestroyUBO(PlanetUBO);
	}

	void Update()
	{
	}

	void SetParameters(game::state * State)
	{
		float sc = 10.0;
		PlanetParams.ModelMatrix.FromTRS(vec3f(0, 0, 0), vec3f(0, 0, 0), vec3f(sc));
		PlanetParams.ViewMatrix = State->Camera.ViewMatrix;
		PlanetParams.CameraPosition = State->Camera.Position;

		PlanetParams.GridW = PlanetParams.GridH = 8;
		PlanetParams.GridD = 6;
		PlanetParams.TileSize = vec3f(0.5f, 1.0f, 0.5f);
		PlanetParams.Origin = vec3f(-PlanetParams.TileSize.x*PlanetParams.GridW*0.5f,
									-PlanetParams.TileSize.x*PlanetParams.GridW*0.5f, 
									-PlanetParams.TileSize.z*PlanetParams.GridH*0.5f);

		PlanetParams.OuterTessFactor = 32.0f;
		PlanetParams.InnerTessFactor = 32.0f;


		static float st = 0.0f;
		st += (float)State->dTime;
		PlanetParams.Time = st;
	}

	void Render(game::state * State, rf::context * Context)
	{
		rf::ctx::SetCullMode(Context);
		//rf::ctx::SetWireframeMode(Context);

		glUseProgram(TessTestShader);

		rf::CheckGLError("PlanetUBOStart");
		SetParameters(State);

		rf::BindUBO(PlanetUBO, 1);
		rf::FillUBO(0, sizeof(planet_params), &PlanetParams);

		rf::CheckGLError("PlanetUBO");
		glPatchParameteri(GL_PATCH_VERTICES, 1);
		glBindVertexArray(PlanetMesh.VAO);
		int instances = PlanetParams.GridW * PlanetParams.GridH * PlanetParams.GridD;
		glDrawElementsInstanced(GL_PATCHES, PlanetMesh.IndexCount, PlanetMesh.IndexType, 0, instances);
		

		rf::CheckGLError("PlanetDraw");
		//rf::ctx::SetWireframeMode(Context);
		rf::ctx::SetCullMode(Context);
	}

	void ReloadShaders(rf::context * Context)
	{
		path const &ExePath = rf::ctx::GetExePath(Context);
		path VSPath, FSPath, TESCPath, TESEPath;
		rf::ConcatStrings(VSPath, ExePath, "data/shaders/planet_vert.glsl");
		rf::ConcatStrings(FSPath, ExePath, "data/shaders/planet_frag.glsl");
		rf::ConcatStrings(TESCPath, ExePath, "data/shaders/planet_tesc.glsl");
		rf::ConcatStrings(TESEPath, ExePath, "data/shaders/planet_tese.glsl");
		TessTestShader = rf::BuildShader(Context, VSPath, FSPath, NULL, TESCPath, TESEPath);
		glUseProgram(TessTestShader);
		rf::ctx::RegisterShader3D(Context, TessTestShader);
		rf::CheckGLError("PlanetShader");
	}
}