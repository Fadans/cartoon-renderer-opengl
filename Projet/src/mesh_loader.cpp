
#include "mesh.hh"
#include <cstdio>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static void process_node(aiNode* node, const aiScene* scene,
                         Mesh* m, aiMatrix4x4 parent_transform,
                         const std::string& dir) {
    aiMatrix4x4 transform = parent_transform * node->mTransformation;

    for (unsigned int mi = 0; mi < node->mNumMeshes; mi++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[mi]];
        int start = m->vertex_count();

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int idx = face.mIndices[j];
                aiVector3D pos = transform * mesh->mVertices[idx];
                float nx = 0, ny = 1, nz = 0;
                if (mesh->HasNormals()) {
                    aiMatrix3x3 normalMat(transform); normalMat.Transpose();
                    aiVector3D nor = normalMat * mesh->mNormals[idx]; nor.Normalize();
                    nx = nor.x; ny = nor.y; nz = nor.z;
                }
                float u = 0, v = 0;
                if (mesh->mTextureCoords[0]) {
                    u = mesh->mTextureCoords[0][idx].x;
                    v = mesh->mTextureCoords[0][idx].y;
                }
                m->push_vertex(pos.x, pos.y, pos.z, nx, ny, nz, u, v);
            }
        }

        std::string tex;
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString tex_name;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex_name) == AI_SUCCESS)
                tex = dir + tex_name.C_Str();
        }
        m->push_submesh(start, m->vertex_count() - start, tex);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        process_node(node->mChildren[i], scene, m, transform, dir);
}

Mesh* Mesh::load_from_file(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->mRootNode) {
        std::fprintf(stderr, "Assimp: impossible de charger %s\n", path.c_str());
        return nullptr;
    }
    std::string dir = path.substr(0, path.find_last_of("/\\") + 1);
    Mesh* m = new Mesh();
    aiMatrix4x4 identity;
    process_node(scene->mRootNode, scene, m, identity, dir);
    std::printf("Loaded %s : %d verts\n", path.c_str(), m->vertex_count());
    return m;
}
