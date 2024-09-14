#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"
class Mesh
{
public:
	Mesh(const char* name, Vertex* vertexArr, size_t vertexNum, unsigned int* indexArr, size_t indexNum);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer(); // Returns the vertex buffer ComPtr
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer(); // Returns the index buffer ComPtr
	int GetIndexCount(); // Returns the number of indices this mesh contains
	int GetVertexCount(); // Returns the number of vertices this mesh contains
	void DrawBuff(); // Sets the buffersand draws using the correct number of indices
		// Refer to Game::Draw() to see the code necessary for setting buffersand drawing

	const char* GetName();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuff;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuff;
	int indexNum;
	int vertexNum;
	void CreateBuffers(Vertex* vertxArr, size_t vertexNum, unsigned int* indexArr, size_t indexNum);
	const char* name;
};

