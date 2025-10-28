#include "FbxReader.h"

#include <assert.h>
#include <Utils.h>
#include <DirectXColors.h>
#include <random>

DirectX::XMFLOAT4 randomColors[] =
{
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::White),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Black),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Red),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Green),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Blue),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Yellow),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Cyan),
    DirectX::XMFLOAT4((const float*)&DirectX::Colors::Magenta)
};

FBXReader::FBXReader()
    : mpManager(nullptr),
    mpScene(nullptr),
    mpRootNode(nullptr)
{
    mpManager = FbxManager::Create();
    assert(mpManager);

    //This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(mpManager, IOSROOT);
    mpManager->SetIOSettings(ios);

    mpScene = FbxScene::Create(mpManager, "My Scene");
    assert(mpScene);
}

FBXReader::~FBXReader()
{
    if (mpRootNode) {
        mpRootNode->Destroy();
        mpRootNode = nullptr;
    }
    
    if (mpScene) {
        mpScene->Destroy();
        mpScene = nullptr;
    }

    if (mpManager) {
        mpManager->Destroy();
        mpManager = nullptr;
    }
}

bool FBXReader::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
    int fileMajor, fileMinor, fileRevision;
    int SDKMajor, SDKMinor, SDKRevision;
    bool status;

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);

    FbxImporter* importer = FbxImporter::Create(pManager, "");
    const bool importStatus = importer->Initialize(pFilename, -1, pManager->GetIOSettings());
    importer->GetFileVersion(fileMajor, fileMinor, fileRevision);

    if (!importStatus)
    {
        FbxString error = importer->GetStatus().GetErrorString();
        ASSERT(importStatus, "Call to FbxImporter::Initialize() failed. Error returned: " << error.Buffer());

        /*if (importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", SDKMajor, SDKMinor, SDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, fileMajor, fileMinor, fileRevision);
        }*/

        return false;
    }

    status = importer->Import(pScene);

    if (status == false && importer->GetStatus() == FbxStatus::ePasswordError)
    {
        ASSERT(status, "Call to importer->Import(pScene) failed. File has password");
    }

    ASSERT(status && (importer->GetStatus() == FbxStatus::eSuccess),
        "FBX Importer failed to load the file. Last error message: " << importer->GetStatus().GetErrorString());

    importer->Destroy();

    return status;
}

bool FBXReader::LoadFbxFile(const std::string& filename)
{
    bool result = false;

    result = LoadScene(mpManager, mpScene, filename.c_str());

    ASSERT(result, "An error occurred while loading the scene.");
    ASSERT(mpScene, "null scene");

    mpRootNode = mpScene->GetRootNode();
}

void FBXReader::GetMeshData(FbxNode* pNode, UINT shift, std::vector<Vertex>& vertices, std::vector<UINT>& indices)
{
    if (!pNode)
        return;

    static int meshNum = 0;

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 7); // distribution in range [0, 7]

    FbxMesh* mesh = pNode->GetMesh();
    if (mesh)
    {
        LOG("Mesh ", meshNum++);

        // Extract vertex positions (control points)
        int numVertices = mesh->GetControlPointsCount();
        FbxVector4* controlPoints = mesh->GetControlPoints();
        for (int i = 0; i < numVertices; ++i)
        {
            Vertex vertex;
            vertex.Pos.x = controlPoints[i][0];
            vertex.Pos.y = controlPoints[i][1];
            vertex.Pos.z = controlPoints[i][2];
            
            vertex.Color = randomColors[dist6(rng)];

            vertices.push_back(vertex);
        }

        int numPolygons = mesh->GetPolygonCount();
        for (int i = 0; i < numPolygons; ++i)
        {
            int numPolygonVertices = mesh->GetPolygonSize(i);
            if (numPolygonVertices == 3)
            {
                indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, 0) + shift));
                indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, 1) + shift));
                indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, 2) + shift));
            }
            else
            {
                // Split into triangles
                for (int j = 2; j < numPolygonVertices; ++j)
                {
                    indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, 0) + shift));
                    indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, j - 1) + shift));
                    indices.push_back(static_cast<UINT>(mesh->GetPolygonVertex(i, j) + shift));
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////
        FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();
        if (normalElement)
        {
            //mapping mode is by control points. The mesh should be smooth and soft.
            if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                for (int vertexIndex = 0; vertexIndex < mesh->GetControlPointsCount(); vertexIndex++)
                {
                    int normalIndex = 0;
                    //reference mode is direct, the normal index is same as vertex index.
                    if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                        normalIndex = vertexIndex;

                    //reference mode is index-to-direct, get normals by the index-to-direct
                    if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                        normalIndex = normalElement->GetIndexArray().GetAt(vertexIndex);

                    FbxVector4 lNormal = normalElement->GetDirectArray().GetAt(normalIndex);
                    //add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
                    //. . .
                }
            }
            //mapping mode is by polygon-vertex.
            else if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                int indexByPolygonVertex = 0;
                for (int polygonIndex = 0; polygonIndex < mesh->GetPolygonCount(); polygonIndex++)
                {
                    LOG(" Polygon ", polygonIndex);
                    int polygonSize = mesh->GetPolygonSize(polygonIndex);
                    for (int i = 0; i < polygonSize; i++)
                    {
                        int normalIndex = 0;
                        //reference mode is direct, the normal index is same as indexByPolygonVertex.
                        if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            normalIndex = indexByPolygonVertex;

                        //reference mode is index-to-direct, get normals by the index-to-direct
                        if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                            normalIndex = normalElement->GetIndexArray().GetAt(indexByPolygonVertex);

                        FbxVector4 normal = normalElement->GetDirectArray().GetAt(normalIndex);
                        LOG("  Vertex ", mesh->GetPolygonVertex(polygonIndex, i), " - Normal (", normal[0], ", ", normal[1], ", ", normal[2], ")");
                        //add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
                        //. . .

                        indexByPolygonVertex++;
                    }
                }

            }
        }

        ////////////////////////////////////////////////////////////////////////////////////
    }

    int i, count = pNode->GetChildCount();
    for (i = 0; i < count; i++)
    {
        GetMeshData(pNode->GetChild(i), vertices.size(), vertices, indices);
    }
}

void FBXReader::GetVertices(std::vector<Vertex>& vertices, std::vector<UINT>& indices)
{
    GetMeshData(mpRootNode, 0, vertices, indices);
}