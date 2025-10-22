#pragma once

#include <string>
#include <vector>
#include <fbxsdk.h>
#include <RenderDefs.h>

class FBXReader
{
public:
    FBXReader();
    ~FBXReader();
    bool LoadFbxFile(const std::string& filename);
    void GetVertices(std::vector<Vertex>& vertices, std::vector<UINT>& indices);

private:
    bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
    void GetMeshData(FbxNode* pNode, UINT shift, std::vector<Vertex>& vertices, std::vector<UINT>& indices);

    FbxManager* mpManager;
    FbxScene* mpScene;
    FbxNode* mpRootNode;
};