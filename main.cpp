#include "src/device.h"
#include "imgui/imgui.h"

class Scene2D : public Scene
{
public:
	bool Init() override
	{
		if ( !Scene::Init() )
			return false;
		const Device &device = GetDevice();
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
		Render::ClearBuffers();

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