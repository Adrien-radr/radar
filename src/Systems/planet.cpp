#include "planet.h"
#include "rf/context.h"
#include "rf/utils.h"
#include "Game/sun.h"

namespace planet
{
	rf::mesh TessTest = {};
	uint32 TessTestShader = 0;

	void Init(game::state * State, rf::context * Context)
	{
		std::vector<vec3f> Positions;
		std::vector<uint32> Indices;

		uint32 vCount = 3;

		vec3f Tr(-0.5f, 0.f, -0.5f);
		Positions.push_back(vec3f(-1.0f, 0.0f, 0.0f));
		Positions.push_back(vec3f(0.0f, 0.0f, 1.0f));
		Positions.push_back(vec3f(1.0f, 0.0f, 0.0f));

		Indices.push_back(0);
		Indices.push_back(1);
		Indices.push_back(2);

		TessTest.IndexCount = 3;
		TessTest.IndexType = GL_UNSIGNED_INT;
		TessTest.VAO = rf::MakeVertexArrayObject();
		TessTest.VBO[0] = rf::AddIBO(GL_STATIC_DRAW, TessTest.IndexCount * sizeof(uint32), &Indices[0]);
		TessTest.VBO[1] = rf::AddEmptyVBO(vCount * sizeof(vec3f), GL_STATIC_DRAW);
		rf::FillVBO(0, 3, GL_FLOAT, 0, vCount * sizeof(vec3f), &Positions[0]);
		glBindVertexArray(0);

	}

	void Update()
	{
	}

	void Render(game::state * State, rf::context * Context)
	{
		mat4f const &ViewMatrix = State->Camera.ViewMatrix;

		rf::ctx::SetCullMode(Context);

		glUseProgram(TessTestShader);
		glPatchParameteri(GL_PATCH_VERTICES, 3);

		{
			uint32 Loc = glGetUniformLocation(TessTestShader, "ViewMatrix");
			rf::SendMat4(Loc, ViewMatrix);
			rf::CheckGLError("ViewMatrix");
		}

		glBindVertexArray(TessTest.VAO);
		rf::RenderMesh(&TessTest, GL_PATCHES);

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
	}
}