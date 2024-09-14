#include "Mesh.h"
#include "Graphics.h"


Mesh::Mesh(const char* name, Vertex* vertexArr, size_t vertexNum, unsigned int* indexArr, size_t indexNum) : 
	name(name)
{
	CreateBuffers(vertexArr, vertexNum, indexArr, indexNum);
}

Mesh::~Mesh()
{

}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer() { return vertexBuff; }
Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer() { return indexBuff; }
const char* Mesh::GetName() { return name; }
int Mesh::GetIndexCount() { return indexNum; }
int Mesh::GetVertexCount() { return vertexNum; }

void Mesh::CreateBuffers(Vertex* vertexArr, size_t vertexNum, unsigned int* indexArr, size_t indexNum)
{
	// Create the vertex buffer
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * (UINT)vertexNum; // Number of vertices
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialVertexData = {};
	initialVertexData.pSysMem = vertexArr;
	Graphics::Device->CreateBuffer(&vbd, &initialVertexData, vertexBuff.GetAddressOf());

	// Create the index buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * (UINT)indexNum; // Number of indices
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialIndexData = {};
	initialIndexData.pSysMem = indexArr;
	Graphics::Device->CreateBuffer(&ibd, &initialIndexData, indexBuff.GetAddressOf());

	// Save the counts
	this->indexNum = (unsigned int)indexNum;
	this->vertexNum = (unsigned int)vertexNum;
}

void Mesh::DrawBuff()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	Graphics::Context->IASetVertexBuffers(0, 1, vertexBuff.GetAddressOf(), &stride, &offset);
	Graphics::Context->IASetIndexBuffer(indexBuff.Get(), DXGI_FORMAT_R32_UINT, 0);
	Graphics::Context->DrawIndexed(this->indexNum, 0, 0);
}
