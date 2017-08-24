#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

static int GetAttribIndex(const std::string &AttribName)
{
    if(AttribName == "POSITION") return 0;
    if(AttribName == "TEXCOORD_0") return 1;
    if(AttribName == "NORMAL") return 2;
    if(AttribName == "TANGENT") return 3;
    else
    {
        printf("Undefined GLTF attrib name %s\n", AttribName.c_str()); 
        return -1;
    }
}

static int GetAttribStride(int AccessorType) {
    switch(AccessorType)
    {
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 4;
        case TINYGLTF_TYPE_MAT3: return 9;
        case TINYGLTF_TYPE_MAT4: return 16;
        default: printf("Wrong GLTF TYPE!\n"); return -1;
    }
}


bool LoadGLTFModel(model *Model, const std::string &Filename, game_context &Context)
{
    tinygltf::Model Mdl;
    tinygltf::TinyGLTF Loader;
    std::string LoadErr;
    
    using namespace tinygltf;

    bool Ret = Loader.LoadASCIIFromFile(&Mdl, &LoadErr, Filename.c_str());
    if(!LoadErr.empty())
    {
        printf("Error loading glTF model %s\n", Filename.c_str());
        return false;
    }

    if(!Ret)
    {
        printf("Failed to parse glTF model %s\n", Filename.c_str());
        return false;
    }

    if(Mdl.meshes.size() > 1)
    {
        printf("UNIMPLEMENTED : glTF model with several meshes %s (%zu)\n", Filename.c_str(), Mdl.meshes.size());
        return false;
    }
    if(Mdl.meshes[0].primitives.size() > 1)
    {
        printf("UNIMPLEMENTED : glTF model with several primitives %s (%zu)\n", Filename.c_str(), Mdl.meshes[0].primitives.size());
        return false;
    }
    if(Mdl.buffers.size() > 1)
    {
        printf("UNIMPLEMENTED : glTF model with several buffers %s (%zu)\n", Filename.c_str(), Mdl.buffers.size());
        return false;
    }
    if(Mdl.materials.size() > 1)
    {
        printf("UNIMPLEMENTED : glTF model with several materials %s (%zu)\n", Filename.c_str(), Mdl.materials.size());
        return false;
    }

    // Load Mesh

    const Primitive &prim = Mdl.meshes[0].primitives[0];
    const Accessor &indices =  Mdl.accessors[prim.indices];
    const BufferView &indicesBV = Mdl.bufferViews[indices.bufferView];
    const Buffer &DataBuffer = Mdl.buffers[0];

    Model->Mesh.IndexCount = indices.count;
    Model->Mesh.IndexType = indices.componentType;
    Model->Mesh.VAO = MakeVertexArrayObject();
    Model->Mesh.VBO[0] = AddIBO(GL_STATIC_DRAW, indicesBV.byteLength, &DataBuffer.data[0] + indices.byteOffset + indicesBV.byteOffset);
    uint32 BufferSize = DataBuffer.data.size() - indicesBV.byteLength;
    Model->Mesh.VBO[1] = AddEmptyVBO(BufferSize, GL_STATIC_DRAW);

    bool ValidMesh[3] = { false, false, false };
    int Attrib = 0;
    for(auto &it : prim.attributes)
    {
        const Accessor &acc = Mdl.accessors[it.second];
        const BufferView & bv = Mdl.bufferViews[acc.bufferView];

        int AttribIdx = GetAttribIndex(it.first);
        int AttribStride = GetAttribStride(acc.type);

        //printf("%s : Idx %d, Stride %d, Size %d\n", it.first.c_str(), AttribIdx, AttribStride, bv.byteLength);

        if(AttribStride < 0 || AttribIdx < 0)
        {
            printf("Error loading glTF model %s : Attrib %d stride or size invalid.\n", Filename.c_str(), Attrib);
            return false;
        }

        FillVBO(AttribIdx, AttribStride, acc.componentType, acc.byteOffset + bv.byteOffset - indicesBV.byteLength, bv.byteLength, 
                &DataBuffer.data[0] + acc.byteOffset + bv.byteOffset);

        if(AttribIdx >= 0 && AttribIdx <= 2)
        {
            ValidMesh[AttribIdx] = true;
        }

        Attrib++;
    }

    if(!ValidMesh[0])
    {
        printf("Error loading glTF model %s : Positions are not given.\n", Filename.c_str());
        return false;
    }
    if(!ValidMesh[1])
    {
        printf("Error loading glTF model %s : Texcoords are not given.\n", Filename.c_str());
        return false;
    }
    if(!ValidMesh[2])
    {
        printf("Error loading glTF model %s : Normals are not given.\n", Filename.c_str());
        return false;
    }

    // Load Textures 
    // TODO - Load all textures first and register in a resource manager. Query it afterwards during material loading

    // Load Material
    const Material &material = Mdl.materials[prim.material];
    if(material.values.size() == 0)
    {
        printf("no PBR\n");
        // TODO - Default error material or something
    }
    else
    {
        auto DiffuseTexIdx = material.values.find("baseColorTexture");
        if(DiffuseTexIdx != material.values.end())
        {
            int TextureIndex = (int)DiffuseTexIdx->second.json_double_value.at("index");
            const Image &img = Mdl.images[Mdl.textures[TextureIndex].source];
            Model->Material.AlbedoTexture = Make2DTexture((void*)&img.image[0], img.width, img.height, img.component, 
                                                          false, false, false, Context.GameConfig->AnisotropicFiltering);
        }
        else
        { // NOTE - No diffuse texture : put in the Default
            Model->Material.AlbedoTexture = Context.DefaultDiffuseTexture;
        }

        auto RoughnessTexIdx = material.values.find("metallicRoughnessTexture");
        if(RoughnessTexIdx != material.values.end())
        {
            int TextureIndex = (int)RoughnessTexIdx->second.json_double_value.at("index");
            const Image &img = Mdl.images[Mdl.textures[TextureIndex].source];
            Model->Material.RoughnessTexture = Make2DTexture((void*)&img.image[0], img.width, img.height, img.component, 
                                                          false, false, false, Context.GameConfig->AnisotropicFiltering);
        }
        else
        { // NOTE - No diffuse texture : put in the Default
            Model->Material.RoughnessTexture = Context.DefaultDiffuseTexture;
        }
    }

    glBindVertexArray(0);

    return true;
}
