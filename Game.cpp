#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Memory"
#include "Mesh.h"
#include "BufferStructs.h"

#include <DirectXMath.h>

// This code assumes files are in "ImGui" subfolder!
// Adjust as necessary for your own folder structure and project setup
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Variables for UI
int number = 0;
XMFLOAT4 color(1.0f, 0.0f, 0.5f, 1.0f);
bool isVisable = true;

// Shader color variable for UI access
//std::unique_ptr<int> number = std::make_unique<int>(0);
VertexShaderData vsData = {};
//vsData.colorTint = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
//vsData.offset = XMFLOAT3(0.25f, 0.0f, 0.0f);


// --------------------------------------------------------
// Called once per program, after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
void Game::Initialize()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		Graphics::Context->VSSetShader(vertexShader.Get(), 0, 0);
		Graphics::Context->PSSetShader(pixelShader.Get(), 0, 0);
	}

	// Create a CONSTANT BUFFER to hold data on the GPU for shaders
	// and bind it to the first vertex shader constant buffer register
	{
		// Calculate the size of our struct as a multiple of 16
		unsigned int size = sizeof(VertexShaderData);
		size = (size + 15) / 16 * 16;

		// Describe the constant buffer and create it
		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.ByteWidth = size;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;
		Graphics::Device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());

		// Activate the constant buffer, ensuring it is 
		// bound to the correct slot (register)
		//  - This should match what our shader expects!
		//  - Your C++ and your shaders have to start matching up!
		Graphics::Context->VSSetConstantBuffers(
			0,		// Which slot (register) to bind the buffer to?
			1,		// How many are we activating?  Can set more than one at a time, if we need
			vsConstantBuffer.GetAddressOf());	// Array of constant buffers or the address of just one (same thing in C++)

		// Gives a Beginning value
		vsData.colorTint = XMFLOAT4(1.0f, 0.20f, 0.25f, 0.50f);
		vsData.offset = XMFLOAT3(0.75f, 0.0f, 0.0f);
	}
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		Graphics::Device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		Graphics::Device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	// === Mesh 1 ===
	Vertex verts1[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};
	unsigned int indices1[] = { 0, 1, 2 };

	// Rhombus
	XMFLOAT4 orange = XMFLOAT4(1.0f, 0.65f, 0.0f, 1.0f);  // Orange color
	XMFLOAT4 pink = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);     // Pink color

	Vertex verts2[] =
	{
		{ XMFLOAT3(-0.5f, 0.0f, 0.0f), orange },  // Left vertex
		{ XMFLOAT3(0.0f, 0.5f, 0.0f), pink },     // Top vertex
		{ XMFLOAT3(0.5f, 0.0f, 0.0f), orange },   // Right vertex
		{ XMFLOAT3(0.0f, -0.5f, 0.0f), pink }     // Bottom vertex
	};
	unsigned int indices2[] =
	{
		0, 1, 2,  // First triangle
		0, 2, 3   // Second triangle
	};


	// SUNFLOWER Mesh
	// Create some yellow for the sunflower
	XMFLOAT4 yellow = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // Petals color

	// Petals varibles
	const int petalCount = 20;
	const float petalLength = 0.2f;
	const float radius = 0.5f;

	std::vector<Vertex> petalVertices;
	std::vector<unsigned int> petalIndices;

	for (int i = 0; i < petalCount; i++)
	{
		float angle = (float(i) / petalCount) * XM_2PI;
		float nextAngle = (float(i + 1) / petalCount) * XM_2PI;

		// Define the base and tip of each petal
		float baseX1 = cosf(angle) * radius;
		float baseY1 = sinf(angle) * radius;
		float baseX2 = cosf(nextAngle) * radius;
		float baseY2 = sinf(nextAngle) * radius;
		float tipX = cosf((angle + nextAngle) / 2) * (radius + petalLength);
		float tipY = sinf((angle + nextAngle) / 2) * (radius + petalLength);

		// Adding vertices for the petal
		petalVertices.push_back({ XMFLOAT3(baseX1, baseY1, 0.0f), yellow });
		petalVertices.push_back({ XMFLOAT3(baseX2, baseY2, 0.0f), yellow });
		petalVertices.push_back({ XMFLOAT3(tipX, tipY, 0.0f), yellow });

		// Indices for petal triangle
		unsigned int startIdx = i * 3;
		petalIndices.push_back(startIdx);
		petalIndices.push_back(startIdx + 1);
		petalIndices.push_back(startIdx + 2);
	}

	// Create meshes and add to vector
	// - The ARRAYSIZE macro returns the size of a locally-defined array
	std::shared_ptr<Mesh> mesh1 = std::make_shared<Mesh>("Triangle", verts1, ARRAYSIZE(verts1), indices1, ARRAYSIZE(indices1)); // heavily reference the triangle code
	std::shared_ptr<Mesh> rhombus = std::make_shared<Mesh>("Rhombus", verts2, ARRAYSIZE(verts2), indices2, ARRAYSIZE(indices2));
	std::shared_ptr<Mesh> petalMesh = std::make_shared<Mesh>("Sunflower Petals", petalVertices.data(), petalVertices.size(), petalIndices.data(), petalIndices.size());


	meshes.push_back(mesh1);
	meshes.push_back(rhombus);
	meshes.push_back(petalMesh);

	
}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	UpdateUI(deltaTime);
	BuildUI();
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		float colorValues[4] = { color.x, color.y, color.z, color.w };
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	colorValues);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{
	// DRAW geometry
	// Loop through the game entities and draw each one
	// - Note: A constant buffer has already been bound to
	//   the vertex shader stage of the pipeline (see Init above)
		for (auto& m : meshes)
		{
			//vsData.colorTint = XMFLOAT4(1.0f, 0.20f, 0.25f, 0.50f);
			//vsData.offset = XMFLOAT3(0.75f, 0.0f, 0.00f);

			D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
			Graphics::Context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
			memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
			Graphics::Context->Unmap(vsConstantBuffer.Get(), 0);

			m->DrawBuff();
		}
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}

// --------------------------------------------------------
// Helper method to update the UI in Update
// --------------------------------------------------------
void Game::UpdateUI(float deltaTime)
{
	// Put this all in a helper method that is called from Game::Update()
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);
	if (isVisable)
	{
		// Show the demo window
		ImGui::ShowDemoWindow();
	}
}

// --------------------------------------------------------
// Helper method to build the UI in Update
// --------------------------------------------------------
void Game::BuildUI()
{
	/*
	X A custom window (named something other than Debug)
	X The current Framerate, which can be retrieved with ImGui::GetIO().Framerate
	X The window dimensions, from the Window’s Width() and Height() functions
	X A color picker for background color using ImGui::ColorEdit4()
	X Currently, the color of the window is defined by a local variable in Game::Draw()
	X Devise a way to store that value between frames and edit it with the UI
	X Hint: The data type can remain a 4-element array of floats, but it can’t be const anymore
	X A button that will hide or show the ImGui demo window when clicked
	X Hint: You’ll need to track its visibility yourself and conditionally call ShowDemoWindow()
	X Several other elements of your choosing for testing (which may be removed in the future)
	*/
	
	ImGui::Begin("Delaney's Epic UI :)");
	ImGui::Text("Hi hi Welcome!!! :0");
	ImGui::StyleColorsClassic();

	// Displays framerate to UI
	ImGui::Text("Framerate: %f fps", ImGui::GetIO().Framerate);

	// Displays W X H to UI
	ImGui::Text("Window Resolution: %dx%d", Window::Width(), Window::Height());

	// Toggles visability of Demo on click
	if (ImGui::Button("Toggle ImGui Demo"))
	{
		isVisable = !isVisable;
	}

	// Color editor for the background
	ImGui::ColorEdit4("Background Color", &color.x);
	int totalVertex = 0;
	int totalTri = 0;

	// Goes through each mesh and displays it's information
	for (auto& m : meshes)
	{
		//ImGui::Text("Mesh: %d", m->GetName(), m->GetIndexCount());
		ImGui::Text(m->GetName());
		ImGui::Text("	Tri Count: %d", m->GetIndexCount() / 3);
		ImGui::Text("	Vertex Count: %d", m->GetVertexCount());

		totalTri += m->GetIndexCount() / 3;
		totalVertex += m->GetVertexCount();
	}

	// Tells about the total geometry on screen
	ImGui::Text("TOTAL Tri: %d", totalTri);
	ImGui::Text("TOTAL Vertex: %d", totalVertex);

	// RGBA sliders
	ImGui::SliderFloat("Red", &vsData.colorTint.x, 0.0f, 1.0f);
	ImGui::SliderFloat("Green", &vsData.colorTint.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Blue", &vsData.colorTint.z, 0.0f, 1.0f);
	ImGui::SliderFloat("Alpha", &vsData.colorTint.w, 0.0f, 1.0f);

	// Offset sliders
	ImGui::SliderFloat("X offset", &vsData.offset.x, -1.0f, 1.0f);
	ImGui::SliderFloat("Y offset", &vsData.offset.y, -1.0f, 1.0f);
	ImGui::SliderFloat("Z offset", &vsData.offset.z, -1.0f, 1.0f);


	// Adds smilies when clicked
	if (ImGui::Button("+1 Smiley"))
	{
		number++;
	}

	// Minuses smilies when clicked
	if (ImGui::Button("-1 Smiley"))
	{
		number--; 
	}

	// A slider to increase the number of smileys
	ImGui::SliderInt("How many Smileys?", &number, 0, 100);

	// Playing with colored text
	ImGui::TextColored(ImVec4(color.x, color.y, color.z, color.w), "Your Smiley Garden");
	
	// Creates a child section which is scrollable within the larger UI
	ImGui::BeginChild("Smiley Garden");
	for (int n = 0; n < number; n++)
		ImGui::Text("%4d: :)", n);
	ImGui::EndChild();

	ImGui::End();

}
