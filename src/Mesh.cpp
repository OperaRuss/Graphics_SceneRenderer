//
// Created by russ on 10/28/21.
//

#include "Mesh.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices) {
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    numVerts = (int)this->vertices.size();
    numEdges = (int)this->indices.size();
    setupMesh();
}

Mesh::Mesh(const std::string &filePath) {
    minPoint = glm::vec3(std::numeric_limits<float>::max());
    maxPoint = glm::vec3(std::numeric_limits<float>::min());
    fromFile(filePath);
    normalizeCoordinates();
    normalizeColors();
    calculateNormals();
    setupMesh();
}

void Mesh::setupMesh() {
    // The Mesh VBO stores data in the following format:
    // [ x y z norm_x norm_y norm_z tex_0 tex_1 ]
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);

    //vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    //vertex colors
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    //vertex normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    //vertex texture
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
}

void Mesh::terminate() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::draw(Shader &shader) {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,(GLsizei)indices.size(),GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::fromFile(const std::string &file_name) {
    std::ifstream in (file_name);
    std::string line;

    if(!in.is_open()){
        std::cout << "ERROR::MESH::LOAD_MESH::FILE_NOT_OPEN" << std::endl;
        exit(-1);
    }

    if(!getNextLine(in, line)){
        return;
    }

    if(line != "OFF"){
        std::cout << "ERROR::MESH::LOAD_MESH::FILE_NOT_SUPPORTED" <<std::endl;
    }

    if(!getNextLine(in, line)){
        return;
    }

    std::istringstream data;
    data.str(line);

    data >> numVerts >> numFaces >> numEdges;
    char c;
    vertices.reserve(numVerts);
    for(int i = 0; i < numVerts; ++i){
        if(!getNextLine(in, line)){
            return;
        }
        data.str(std::string{});
        data.clear();
        data.str(line);
        glm::vec3 temp;
        data >> temp.x >> temp.y >> temp.z;
        vertices.emplace_back(Vertex(temp,glm::vec3(150,150,150)));
        boundingCheck(temp);
    }

    GLuint edges;
    GLuint index;
    GLuint red, green, blue;
    for(int i = 0; i < numFaces; ++i){
        data.str(std::string{});
        data.clear();
        if(!getNextLine(in, line)){
            return;
        }
        data.str(line);
        data >> edges;
        for(int ii = 0; ii < 3; ++ii){
            data >> index;
            indices.push_back(index);
        }

        if(!data.eof()){
            data >> red >> green >> blue;
            vertices[i].color = glm::vec3(red,green,blue);
        }
    }

    in.close();
    return;
}

bool Mesh::getNextLine(std::ifstream &infile, std::string &line) {
    while(getline(infile,line)){
        if(line.length() > 1 && line[0] != '#')
            return true;
    }
    return false;
}

void Mesh::boundingCheck(const glm::vec3 &point) {
    if(point.x < minPoint.x)
        minPoint.x = point.x;
    if(point.x > maxPoint.x)
        maxPoint.x = point.x;
    if(point.y < minPoint.y)
        minPoint.y = point.y;
    if(point.y > maxPoint.y)
        maxPoint.y = point.y;
    if(point.z < minPoint.z)
        minPoint.z = point.z;
    if(point.z > maxPoint.z)
        maxPoint.z = point.z;
}

void Mesh::normalizeCoordinates() {
    glm::vec3 pointSpan = maxPoint - minPoint;
    glm::vec3 unitMinPoint = glm::vec3(-0.5,-0.5,-0.5);

    for(auto& vertex : vertices){
        vertex.position.x = unitMinPoint.x + ((vertex.position.x - minPoint.x)/pointSpan.x);
        vertex.position.y = unitMinPoint.y + ((vertex.position.y - minPoint.y)/pointSpan.y);
        vertex.position.z = unitMinPoint.z + ((vertex.position.z - minPoint.z)/pointSpan.z);
    }

    for(const auto& vertex: vertices){
        boundingCheck(vertex.position);
    }
}

void Mesh::normalizeColors() {
    for(auto& vertex : vertices){
        vertex.color.x /= 255;
        vertex.color.y /= 255;
        vertex.color.z /= 255;
    }
}

bool Mesh::isHit(const glm::vec3& raySource, const glm::vec3& rayDir, const glm::mat4& modelMatrix) const{
    glm::vec3 rayEnd = raySource + (rayDir * 1000.0f);
    // for each face
    for(int m = 0; m < numFaces; ++m){
        glm::vec3 v[3];
        for(int ii = 0; ii < 3; ++ii){
            // As we have EBO format, extract 3 verts for each face
            int index = (3 * m) + ii;
            v[ii] = glm::vec3(modelMatrix * glm::vec4(vertices[indices[index]].position,1.0f));
        }
        // Using Craemer's rule for determinants, calculate possible intersection
        GLfloat a, b, c, d, e, f, g, h, i, j, k, l;
        a = v[0].x - v[1].x;
        b = v[0].y - v[1].y;
        c = v[0].z - v[1].z;
        d = v[0].x - v[2].x;
        e = v[0].y - v[2].y;
        f = v[0].y - v[2].z;
        g = rayDir.x;
        h = rayDir.y;
        i = rayDir.z;
        j = v[0].x - raySource.x;
        k = v[0].y - raySource.y;
        l = v[0].z - raySource.z;

        GLfloat eihf, gfdi, dheg, akjb, jcal, blkc;
        eihf = (e * i) - (h * f);
        gfdi = (g * f) - (d * i);
        dheg = (d * h) - (e * g);
        akjb = (a * k) - (j * b);
        jcal = (j * c) - (a * l);
        blkc = (b * l) - (k * c);

        GLfloat M = (a * eihf) + (b * gfdi) + (c * dheg);
        GLfloat t = - (((f*akjb)+(e*jcal)+(d*blkc))/M);
        // Test iteratively to save some computations.
        // (I'm sure there was a way to do this with less overhead!)
        if (t > raySource.z || t < rayEnd.z)
            continue;
        GLfloat gamma = (((i*akjb)+(h*jcal)+(g*blkc))/M);
        if (gamma < 0 || gamma > 1)
            continue;
        GLfloat beta = (((j*eihf)+(k*gfdi)+(l*dheg))/M);
        if (beta < 0 || beta > (1 - gamma))
            continue;
        return true;
    }
    return false;
}

void Mesh::calculateNormals() {
    // there are equal vertices and normals
    normals.resize(numVerts);
    std::vector<GLuint> numAdjacentFaces;
    numAdjacentFaces.resize(numVerts);

    // read in vertex data, indexed by EBO
    for(int i = 0; i < numFaces; ++i) {
        glm::vec3 v[3];
        std::vector<GLuint> indxs;
        for (int ii = 0; ii < 3; ++ii) {
            // As we have EBO format, extract 3 verts for each face
            GLuint index = (3 * i) + ii;
            v[ii] = glm::vec3(glm::vec4(vertices[indices[index]].position, 1.0f));
            indxs.push_back(indices[index]);
        }

        // create normal
        glm::vec3 normal;
        normal = glm::normalize(glm::cross((v[1] - v[0]),(v[2] - v[0])));

        // add normal value to each index
        for(GLuint idx : indxs){
            normals[idx] += normal;
            ++numAdjacentFaces[idx];
        }
        indxs.clear();
    }

    // use counters to obtain Euclidean average over num faces
    for(int i = 0; i < numVerts; ++i){
        vertices[i].normal = glm::normalize(normals[i] / (float)numAdjacentFaces[i]);
    }
}

void Mesh::setTexCoords(int vertIndex, glm::vec2 texCoord) {
    vertices[vertIndex].texCoord = texCoord;
}


