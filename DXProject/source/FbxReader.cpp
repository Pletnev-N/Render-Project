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
    return result;
}

void FBXReader::GetMeshDataOld(FbxNode* pNode, UINT shift, std::vector<VertexTextured>& vertices, std::vector<UINT>& indices)
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
            VertexTextured vertex;
            vertex.Pos.x = controlPoints[i][0];
            vertex.Pos.y = controlPoints[i][1];
            vertex.Pos.z = controlPoints[i][2];
            
            //vertex.Color = randomColors[dist6(rng)];

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
            FbxVector4 normal;

            //mapping mode is by control points. The mesh should be smooth and soft.
            if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                ASSERT(false, "Not implemented");
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
                    //LOG(" Polygon ", polygonIndex);
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

                        normal = normalElement->GetDirectArray().GetAt(normalIndex);
                        //LOG("  Vertex ", mesh->GetPolygonVertex(polygonIndex, i), " - Normal (", normal[0], ", ", normal[1], ", ", normal[2], ")");

                        vertices[mesh->GetPolygonVertex(polygonIndex, i) + shift].Normal.x = static_cast<float>(normal[0]);
                        vertices[mesh->GetPolygonVertex(polygonIndex, i) + shift].Normal.y = static_cast<float>(normal[1]);
                        vertices[mesh->GetPolygonVertex(polygonIndex, i) + shift].Normal.z = static_cast<float>(normal[2]);

                        indexByPolygonVertex++;
                    }
                }

            }
        }

        //////////////////////////////////////////////////////////////////

        for (int i = 0; i < mesh->GetElementUVCount(); ++i)
        {
            FbxGeometryElementUV* elementUV = mesh->GetElementUV(i);
            FbxVector2 uv;

            if (elementUV->GetMappingMode() == FbxGeometryElement::eByControlPoint)
            {
                ASSERT(false, "Not implemented");
                for (int vertexIndex = 0; vertexIndex < mesh->GetControlPointsCount(); vertexIndex++)
                {
                    if (elementUV->GetReferenceMode() == FbxGeometryElement::eDirect)
                    {
                        uv = elementUV->GetDirectArray().GetAt(vertexIndex);
                    }
                    else if (elementUV->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                    {
                        int id = elementUV->GetIndexArray().GetAt(vertexIndex);
                        uv = elementUV->GetDirectArray().GetAt(id);
                    }
                }
            }
            else if (elementUV->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                for (int polygonIndex = 0; polygonIndex < mesh->GetPolygonCount(); polygonIndex++)
                {
                    LOG(" Polygon ", polygonIndex);
                    int polygonSize = mesh->GetPolygonSize(polygonIndex);
                    for (int i = 0; i < polygonSize; i++)
                    {
                        int textureUVIndex = mesh->GetTextureUVIndex(polygonIndex, i);
                        if (elementUV->GetReferenceMode() == FbxGeometryElement::eDirect ||
                            elementUV->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                        {
                            uv = elementUV->GetDirectArray().GetAt(textureUVIndex);
                        }

                        int id = mesh->GetPolygonVertex(polygonIndex, i) + shift;
                        LOG("  Vertex ",id , " - Pos (", vertices[id].Pos.x, ", ", vertices[id].Pos.y, ", ", vertices[id].Pos.z,
                            ") - UV (", uv[0], ", ", uv[1], ")");
                        vertices[mesh->GetPolygonVertex(polygonIndex, i) + shift].Tex.x = static_cast<float>(uv[0]);
                        vertices[mesh->GetPolygonVertex(polygonIndex, i) + shift].Tex.y = static_cast<float>(uv[1]);
                    }
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////
    }

    LOG("vertices size = ", vertices.size());
    for (int i =0 ; i < vertices.size(); i++)
    {
        LOG(" Vertex ", i, ": Pos(", vertices[i].Pos.x, ", ", vertices[i].Pos.y, ", ", vertices[i].Pos.z,
            ") Tex(", vertices[i].Tex.x, ", ", vertices[i].Tex.y, ")");
    }

    int i, count = pNode->GetChildCount();
    for (i = 0; i < count; i++)
    {
        GetMeshDataOld(pNode->GetChild(i), vertices.size(), vertices, indices);
    }
}

template<class T>
int GetIndexByReferenceMode(const FbxLayerElementTemplate<T>* element, int vertexIndex)
{
    //reference mode is direct, the normal index is same as vertex index.
    if (element->GetReferenceMode() == FbxGeometryElement::eDirect)
    {
        return vertexIndex;
    }
    //reference mode is index-to-direct, get normals by the index-to-direct
    else if (element->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
    {
        return element->GetIndexArray().GetAt(vertexIndex);
    }
    else
    {
        ASSERT(false, "Unsupported reference mode");
        return 0;
    }
}

FbxVector4 GetNormal(FbxMesh* mesh, int controlPointId, int indexByPolygonVertex)
{
    FbxVector4 normal;
    FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();
    if (normalElement)
    {
        //mapping mode is by control points. The mesh should be smooth and soft.
        if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        {
            ASSERT(false, "Not implemented");
            int normalIndex = GetIndexByReferenceMode(normalElement, controlPointId);
            normal = normalElement->GetDirectArray().GetAt(normalIndex);
            //add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
            //. . .
        }
        //mapping mode is by polygon-vertex.
        else if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int normalIndex = GetIndexByReferenceMode(normalElement, indexByPolygonVertex);
            normal = normalElement->GetDirectArray().GetAt(normalIndex);
        }
    }
    return normal;
}

FbxVector2 GetUV(FbxMesh* mesh, int polygonIndex, int positionInPolygon, int controlPointId)
{
    FbxVector2 uv;
    FbxGeometryElementUV* elementUV = mesh->GetElementUV();
    if (elementUV)
    {
        if (elementUV->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        {
            ASSERT(false, "Not implemented");
            int uvIndex = GetIndexByReferenceMode(elementUV, controlPointId);
            uv = elementUV->GetDirectArray().GetAt(uvIndex);
        }
        else if (elementUV->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int textureUVIndex = mesh->GetTextureUVIndex(polygonIndex, positionInPolygon);
            if (elementUV->GetReferenceMode() == FbxGeometryElement::eDirect ||
                elementUV->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
            {
                uv = elementUV->GetDirectArray().GetAt(textureUVIndex);
            }

            
        }
    }
    return uv;
}

void FBXReader::GetMeshData(FbxNode* pNode, UINT shift, std::vector<VertexTextured>& vertices, std::vector<UINT>& indices)
{
    if (!pNode)
        return;

    static int meshNum = 0;

    //std::random_device dev;
    //std::mt19937 rng(dev());
    //std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 7); // distribution in range [0, 7]

    FbxMesh* mesh = pNode->GetMesh();
    if (mesh)
    {
        LOG("Mesh ", meshNum++);

        // Extract vertex positions (control points)
        int numControlPoints = mesh->GetControlPointsCount();
        FbxVector4* controlPoints = mesh->GetControlPoints();

        // Each element keeps indices to the vertices array where this control point is used
        std::vector< std::vector<int> > controlPointsCopies(numControlPoints);
        for (auto& vec : controlPointsCopies)
        {
            vec.reserve(10);
        }

        std::vector<int> tempIndices;
        tempIndices.reserve(8);
        int indexByPolygonVertex = 0;
        int numPolygons = mesh->GetPolygonCount();

        for (int polygonIndex = 0; polygonIndex < numPolygons; polygonIndex++)
        {
            //LOG(" Polygon ", polygonIndex);
            tempIndices.clear(); // Indices for this polygon
            int polygonSize = mesh->GetPolygonSize(polygonIndex);

            for (int i = 0; i < polygonSize; i++)
            {
                VertexTextured vertex;
                int controlPointId = mesh->GetPolygonVertex(polygonIndex, i);

                vertex.Pos.x = controlPoints[controlPointId][0];
                vertex.Pos.y = controlPoints[controlPointId][1];
                vertex.Pos.z = controlPoints[controlPointId][2];
                
                const FbxVector4& normal = GetNormal(mesh, controlPointId, indexByPolygonVertex);
                //LOG("  Vertex ", controlPointId, " - Normal (", normal[0], ", ", normal[1], ", ", normal[2], ")");
                vertex.Normal.x = static_cast<float>(normal[0]);
                vertex.Normal.y = static_cast<float>(normal[1]);
                vertex.Normal.z = static_cast<float>(normal[2]);

                const FbxVector2& uv = GetUV(mesh, polygonIndex, i, controlPointId);
                //LOG("  Vertex ", controlPointId + shift,
                // " - Pos (", vertices[controlPointId + shift].Pos.x, ", ", vertices[controlPointId + shift].Pos.y, ", ", vertices[controlPointId + shift].Pos.z,
                // ") - UV (", uv[0], ", ", uv[1], ")");
                vertex.Tex.x = static_cast<float>(uv[0]);
                vertex.Tex.y = static_cast<float>(uv[1]);

                indexByPolygonVertex++;

                // Check if this control point was already used for this combination of attributes
                bool found = false;
                const auto& pointCopies = controlPointsCopies[controlPointId];
                if (!pointCopies.empty())
                {
                    for (int j = 0; j < pointCopies.size(); j++)
                    {
                        const VertexTextured& usedVertex = vertices[pointCopies[j]];
                        if (usedVertex.Normal.x == vertex.Normal.x &&
                            usedVertex.Normal.y == vertex.Normal.y &&
                            usedVertex.Normal.z == vertex.Normal.z &&
                            usedVertex.Tex.x == vertex.Tex.x &&
                            usedVertex.Tex.y == vertex.Tex.y)
                        {
                            // Reuse existing vertex
                            tempIndices.push_back(static_cast<UINT>(pointCopies[j]));
                            found = true;
                            break;
                        }
                    }
                }

                if (!found)
                {
                    // Create a new vertex
                    vertices.push_back(vertex);
                    UINT newIndex = static_cast<UINT>(vertices.size() - 1);
                    tempIndices.push_back(newIndex);
                    controlPointsCopies[controlPointId].push_back(newIndex + shift);
                }


            }

            if (tempIndices.size() == 3)
            {
                indices.push_back(static_cast<UINT>(tempIndices[0] + shift));
                indices.push_back(static_cast<UINT>(tempIndices[1] + shift));
                indices.push_back(static_cast<UINT>(tempIndices[2] + shift));
            }
            else
            {
                // Split into triangles
                for (int i = 2; i < tempIndices.size(); ++i)
                {
                    indices.push_back(static_cast<UINT>(tempIndices[0] + shift));
                    indices.push_back(static_cast<UINT>(tempIndices[i - 1] + shift));
                    indices.push_back(static_cast<UINT>(tempIndices[i] + shift));
                }
            }
        }
    }

    LOG("vertices size = ", vertices.size());
    for (int i =0 ; i < vertices.size(); i++)
    {
        LOG(" Vertex ", i, ": Pos(", vertices[i].Pos.x, ", ", vertices[i].Pos.y, ", ", vertices[i].Pos.z,
            ") Tex(", vertices[i].Tex.x, ", ", vertices[i].Tex.y, ")");
    }

    int i, count = pNode->GetChildCount();
    for (i = 0; i < count; i++)
    {
        GetMeshData(pNode->GetChild(i), vertices.size(), vertices, indices);
    }
}

void FBXReader::GetVertices(std::vector<VertexTextured>& vertices, std::vector<UINT>& indices)
{
    GetMeshData(mpRootNode, 0, vertices, indices);
}