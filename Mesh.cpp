#include "Mesh.hpp"

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture2D*> textures, GLenum primitiveType)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
	this->primitiveType = primitiveType;

	// Print all vertices for debugging
	//for (const auto& vertex : vertices) {
	//	Debug::log("Vertex Position: (" + std::to_string(vertex.Position.x) + ", " + std::to_string(vertex.Position.y) + ", " + std::to_string(vertex.Position.z) + ")");
	//}

    // now that we have all the required data, set the vertex buffers and its attribute pointers.
    setupMesh();

    vertices.shrink_to_fit();
	indices.shrink_to_fit();
	textures.shrink_to_fit();
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::Draw(Shader& shader)
{
    // bind appropriate textures
	unsigned int otherNr = 1;
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        // retrieve texture number (the N in diffuse_textureN)
        string number;
        Texture2D::TextureType texType = textures[i]->getType();
		string name = "";
        if (texType == Texture2D::TextureType::TextureDiffuse)
        {
            name = "texture_diffuse";
            number = std::to_string(diffuseNr++);
        }
        else if (texType == Texture2D::TextureType::TextureSpecular)
        {
            name = "texture_specular";
            number = std::to_string(specularNr++); // transfer unsigned int to string
        }
        else if (texType == Texture2D::TextureType::TextureNormal)
        {
            name = "texture_normal";
            number = std::to_string(normalNr++); // transfer unsigned int to string
        }
        else if (texType == Texture2D::TextureType::TextureHeight)
        {
            name = "texture_height";
            number = std::to_string(heightNr++); // transfer unsigned int to string
        }
        else
        {
            name = "texture_other";
            number = std::to_string(otherNr++); // generic numbering for other types
		}

        // now set the sampler to the correct texture unit
		shader.setInt(name + number, i);
        // and finally bind the texture
		textures[i]->bind(i);
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(primitiveType, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
	//shader.stop();

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
    Texture2D::unbind();
	glBindVertexArray(0);
}

void Mesh::setupMesh()
{
    // create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex Positions
	Shader::setAttr(0, 3, sizeof(Vertex), (void*)0);
    // vertex normals
	Shader::setAttr(1, 3, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
	Shader::setAttr(2, 2, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // vertex tangent
	Shader::setAttr(3, 3, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // vertex bitangent
	Shader::setAttr(4, 3, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    // ids
	Shader::setAttr(5, 4, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs), GL_INT);

    // weights
	Shader::setAttr(6, 4, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}