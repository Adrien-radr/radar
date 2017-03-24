#include "src/device.h"
#include "src/fluid.h"
#include "imgui/imgui.h"

static vec3f translations[100];

class Scene2D : public Scene
{
public:
	bool Init() override
	{
		if ( !Scene::Init() )
			return false;
		Device &device = GetDevice();
		const Config &config = device.GetConfig();

		camera.hasMoved = false;
		camera.speedMode = false;
		camera.freeflyMode = false;
		camera.dist = 7.5f;
		camera.speedMult = config.cameraSpeedMult;
		camera.translationSpeed = config.cameraBaseSpeed;
		camera.rotationSpeed = 0.01f * config.cameraRotationSpeed;
		camera.position = config.cameraPosition;
		camera.target = config.cameraTarget;
		camera.up = vec3f( 0, 1, 0 );
		camera.forward = Normalize( camera.target - camera.position );

		camera.right = Normalize( Cross( camera.forward, camera.up ) );
		camera.up = Normalize( Cross( camera.right, camera.forward ) );

		vec2f azimuth = Normalize( vec2f( camera.forward[0], camera.forward[2] ) );
		camera.phi = std::atan2( azimuth[1], azimuth[0] );
		camera.theta = std::atan2( camera.forward[1], std::sqrt( Dot( azimuth, azimuth ) ) );

		// initialize shader matrices
		UpdateView();


		// Create shader for instancing
		using namespace Render;
		Shader::Desc sd_inst;
		sd_inst.vertex_file = "../data/shaders/2d_billboard_inst_vert.glsl";
		sd_inst.fragment_file = "../data/shaders/2d_billboard_inst_frag.glsl";
		sd_inst.projType = Shader::PROJECTION_2D;

		sd_inst.AddAttrib("in_position", 0);
		sd_inst.AddAttrib("in_color", 5);
		sd_inst.AddAttrib("in_offset", 6);
		sd_inst.AddUniform("ProjMatrix", Shader::UNIFORM_PROJMATRIX);

		shader_inst = Shader::Build(sd_inst);
		if(shader_inst < 0)
		{
		    LogErr("Error loading instancing shader.");
		    return false;
		}

		// create quad mesh
		f32 positions[] = {
			-0.05f, 0.05f, 0.f, // topleft
			-0.05f, -0.05f, 0.f, // botleft
			0.05f, -0.05f, 0.f, // botright
			0.05f, 0.05f, 0.f, // topright
		};
		f32 colors[] = {
			1.f, 0.f, 0.f, 1.f,
			1.f, 1.f, 0.f, 1.f,
			0.f, 1.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 1.f
		};
		u32 indices[] = { 0, 1, 2, 0, 2, 3 };

		int ww = config.windowSize.x;
		int wh = config.windowSize.y;

		// init translations array
		{
			int idx = 0;
			f32 off = 0.05f;
			for (int y = 0; y < 10; y += 1)
			{
				for (int x = 0; x < 10; x += 1)
				{
					vec3f translation;
					translation.x = ww * (f32(x) / 10.f + off);
					translation.y = wh * (f32(y) / 10.f + off);
					translation.z = 0.f;
					translations[idx++] = translation;
				}
			}
		}

		Mesh::Desc md("InstQuad", false, 6, indices, 4, positions, nullptr, nullptr, nullptr, nullptr, colors);
		md.SetAdditionalData((f32 *)translations, GL_FLOAT, 3, 100);
		mesh_inst = Mesh::Build(md);

		return true;
	}

	void Update( f32 dt ) override
	{
		camera.Update( dt );
		if ( camera.hasMoved )
		{
			camera.hasMoved = false;
			UpdateView();
		}

		UpdateGUI();
	}

	void UpdateGUI() override
	{
		ImGuiIO &io = ImGui::GetIO();

		// Static info panel
		const float fps = io.Framerate;
		const float mspf = 1000.f / fps;
		const vec3f cpos = camera.position;
		const vec3f ctar = camera.target;

		static char fpsText[512];
		snprintf( fpsText, 512, "Average %.3f ms/frame (%.1f FPS)", mspf, fps );
		const f32 fpsTLen = ImGui::CalcTextSize( fpsText ).x;

		static char camText[512];
		snprintf( camText, 512, "Camera <%.2f, %.2f, %.2f> <%.2f, %.2f, %.2f>", cpos.x, cpos.y, cpos.z, ctar.x, ctar.y, ctar.z );
		const f32 camTLen = ImGui::CalcTextSize( camText ).x;

		static char pickText[512];
		snprintf( pickText, 512, "Pick Object : %d, Vertex : %d", (int) pickedObject, pickedTriangle );
		const f32 pickTLen = ImGui::CalcTextSize( pickText ).x;

		vec2f wSizef = GetDevice().windowSize;
		ImVec2 wSize( wSizef.x, wSizef.y );
		const ImVec2 panelSize( 410, 50 );
		const ImVec2 panelPos( wSize.x - panelSize.x, 19 );

		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImColor::HSV( 0, 0, 0.9f, 0.15f ) );
		ImGui::PushStyleColor( ImGuiCol_Text, ImColor::HSV( 0, 0, 0.4f, 1 ) );
		ImGui::SetNextWindowPos( panelPos );
		ImGui::SetNextWindowSize( panelSize );
		ImGui::Begin( "InfoPanel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );
		ImGui::SameLine( ImGui::GetContentRegionMax().x - fpsTLen );
		ImGui::Text( "%s", fpsText );
		ImGui::Text( "%s", "" );
		ImGui::SameLine( ImGui::GetContentRegionMax().x - camTLen );
		ImGui::Text( "%s", camText );
		ImGui::Text( "%s", "" );
		ImGui::SameLine( ImGui::GetContentRegionMax().x - pickTLen );
		ImGui::Text( "%s", pickText );
		ImGui::End();
		ImGui::PopStyleColor( 2 );
	}

	void Render() override
	{
		const Device &device = GetDevice();

		Render::ClearBuffers();

		// Draw instanced shit
		Render::Shader::Bind(shader_inst);
		Render::Mesh::RenderInstanced(mesh_inst);
		
		// Draw Skybox
		glDisable( GL_CULL_FACE );
		glDepthFunc( GL_LEQUAL );
		Render::Shader::Bind( Render::Shader::SHADER_SKYBOX );
		Render::Mesh::Render( skyboxMesh );
		glDepthFunc( GL_LESS );
		glEnable( GL_CULL_FACE );

	}

	void UpdateView() override
	{
		camera.target = camera.position + camera.forward;
		viewMatrix = mat4f::LookAt( camera.position, camera.target, camera.up );

		Render::UpdateView( viewMatrix );
	}

private:
	Camera camera;
	Render::Shader::Handle shader_inst;
	Render::Mesh::Handle mesh_inst;
};

Scene2D scene;
/*
bool Init(Scene *scene) {
	Skybox::Desc skyd;
	skyd.filenames[0] = "../radar/data/default_diff.png";
	skyd.filenames[1] = "../radar/data/default_diff.png";
	skyd.filenames[2] = "../radar/data/default_diff.png";
	skyd.filenames[3] = "../radar/data/default_diff.png";
	skyd.filenames[4] = "../radar/data/default_diff.png";
	skyd.filenames[5] = "../radar/data/default_diff.png";
	//Skybox::Handle sky = scene->Add(skyd);
	//if (sky < 0) {
		//LogErr("Error loading skybox.");
		//return false;
	//}
	//scene->SetSkybox(sky);

	return true;
}
*/

void MainLoop( float dt )
{
	scene.Update( dt );
	scene.Render();
}

int main() {
	Log::Init();

	Device &device = GetDevice();
	if (!device.Init( MainLoop )) {
		printf("Error initializing Device. Aborting.\n");
		DestroyDevice();
		Log::Close();
		system("PAUSE");
		return 1;
	}

	if ( !scene.Init() )
	{
		printf( "Error initializing Scene. Aborting.\n" );
		DestroyDevice();
		Log::Close();
		system( "PAUSE" );
		return 1;
	}
	
	device.Run();

	DestroyDevice();
	Log::Close();

	return 0;
}
