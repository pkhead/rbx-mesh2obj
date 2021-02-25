// RbxMeshToObj.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

typedef uint8_t byte;
typedef uint32_t uint;

namespace mesh_v1 {
    struct VertexContainer {
        double px, py, pz; // position
        double nx, ny, nz; // normal
        double tu, tv, tw; // uv (v is 1.0 - v, w is unused (0))
    };

    struct FaceContainer {
        int v1, v2, v3;
    };

    struct Tuple {
        double a, b, c;
    };
}

namespace mesh_v2 {
    struct MeshHeader {
        uint16_t sizeof_MeshHeader;
        byte sizeof_Vertex;
        byte sizeof_Face;

        uint numVerts;
        uint numFaces;
    };

    struct Vertex {
        float px, py, pz; // position
        float nx, ny, nz; // unit normal
        float tu, tv, tw; // uv coords (tw is reversed)
        byte r, g, b, a; // color
    };

    struct Face {
        uint a, b, c;
    };
}

#define strEqual(s1, s2) (std::strcmp(s1, s2) == 0)

bool convert(char* sourcePath, std::ifstream& inputFile, std::ofstream& outputFile) {
    //https://devforum.roblox.com/t/roblox-mesh-format/326114

    char version[14];
    version[13] = '\0';
    inputFile.read(version, 13);

    //std::cout << version;

    if (strEqual(version, "version 1.00\n") || strEqual(version, "version 1.01\n")) {
        using namespace mesh_v1;

        double scale = strEqual(version, "version 1.00\n") ? 0.5 : 1.0;

        std::string strNumFaces;
        getline(inputFile, strNumFaces);

        int numFaces = std::stoi(strNumFaces);
        int numVerts = numFaces * 3;
        int numTuples = numVerts * 3;

        // Read tuples //
        Tuple* tuples = new Tuple[numTuples];

        double currTuple[2];
        int n_currTuple = 0;
        int i_currTuple = 0;
        char currChar;
        std::string tupleVal;
        
        while (inputFile.peek() != EOF) {
            inputFile >> currChar;

            if (currChar == ']') {
                tuples[n_currTuple++] = Tuple{ currTuple[0], currTuple[1], std::stod(tupleVal) };
                i_currTuple = 0;
                tupleVal.clear();
                continue;
            }

            if (currChar == '[') continue;

            if (currChar == ',') {
                currTuple[i_currTuple++] = std::stod(tupleVal);
                tupleVal.clear();
                continue;
            }

            tupleVal.push_back(currChar);
        }

        // OUTPUT //
        VertexContainer* vertices = new VertexContainer[numVerts];
        //FaceContainer* faces = new FaceContainer[numFaces];

        for (int i = 0; i < numTuples; i += 3) {
            Tuple* face = tuples + i;
            VertexContainer vtx;

            vtx.px = face[0].a * scale;
            vtx.py = face[0].b * scale;
            vtx.pz = face[0].c * scale;

            vtx.nx = face[1].a;
            vtx.nx = face[1].b;
            vtx.nz = face[1].c;

            vtx.tu = face[2].a;
            vtx.tv = face[2].b;
            vtx.tw = 0.0;

            vertices[i / 3] = vtx;
        }

        outputFile << "# ROBLOX .mesh version 1.0" << (scale == 1.0) << '\n';

        // Write vertex position data
        for (int i = 0; i < numVerts; i++) {
            outputFile << "v " <<
                vertices[i].px << ' ' <<
                vertices[i].py << ' ' <<
                vertices[i].pz << '\n';
        }

        // Write vertex UV data
        for (int i = 0; i < numVerts; i++) {
            outputFile << "vt " <<
                vertices[i].tu << ' ' <<
                vertices[i].tv << '\n';
        }

        // Write vertex normal data
        for (int i = 0; i < numVerts; i++) {
            outputFile << "vn " <<
                vertices[i].nx << ' ' <<
                vertices[i].ny << ' ' <<
                vertices[i].nz << '\n';
        }

        outputFile << "s 1\n";

        // Write faces
        for (int i = 0; i < numVerts; i += 3) {
            int newi = i + 1;
            outputFile << "f " <<
                newi << '/' << newi << '/' << newi << ' ' <<
                (newi + 1) << '/' << (newi + 1) << '/' << (newi + 1) << ' ' <<
                (newi + 2) << '/' << (newi + 2) << '/' << (newi + 2) << '\n';
        }

        return true;
    }

    if (strEqual(version, "version 2.00\n")) {
        using namespace mesh_v2; //woahhhh i've only seen this at the top of the code lol

        //change to binary mode
        inputFile.close();
        inputFile.open(sourcePath, std::ios::binary);
        inputFile.read(version, 13);
        
        MeshHeader header;
        inputFile.read((char*)(&header), sizeof(header));

        if (sizeof(header) != header.sizeof_MeshHeader) {
            std::cout << "ERROR: Mesh header size invalid";
            return false;
        }

        Vertex* vertices = new Vertex[header.numVerts];
        Face* faces = new Face[header.numFaces];

        bool noColor = header.sizeof_Vertex != 40;

        // Read vertex array
        for (int i = 0; i < header.numVerts; i++) {
            inputFile.read((char*)(vertices + i), header.sizeof_Vertex);

            if (noColor) {
                vertices[i].r = vertices[i].g = vertices[i].b = vertices[i].a = 255;
            }
        }

        // Read face array
        for (int i = 0; i < header.numFaces; i++) {
            inputFile.read((char*)(faces + i), header.sizeof_Face);
            faces[i].a++;
            faces[i].b++;
            faces[i].c++; //ahahah i get it
        }

        // OUTPUT //
        outputFile << "# ROBLOX .mesh version 2.00\n";

        // Write vertex positions
        for (int i = 0; i < header.numVerts; i++) {
            Vertex vtx = vertices[i];
            outputFile << "v " << vtx.px << ' ' << vtx.py << ' ' << vtx.pz << '\n';
        }

        // Write vertex UVs
        for (int i = 0; i < header.numVerts; i++) {
            Vertex vtx = vertices[i];
            outputFile << "vt " << vtx.tu << ' ' << vtx.tv << ' ' << '\n';
        }

        // Write vertex normals
        for (int i = 0; i < header.numVerts; i++) {
            Vertex vtx = vertices[i];
            outputFile << "vn " << vtx.nx << ' ' << vtx.ny << ' ' << 1.0 - vtx.nz << '\n';
        }

        outputFile << "s 1\n";

        // Write face data
        for (int i = 0; i < header.numFaces; i++) {
            Face face = faces[i];
            outputFile << "f " <<
                face.a << '/' << face.a << '/' << face.a << ' ' <<
                face.b << '/' << face.b << '/' << face.b << ' ' <<
                face.c << '/' << face.c << '/' << face.c << '\n';
        }

        delete[] vertices;
        delete[] faces;
        
        return true;
    }

    std::cout << "ERROR: Unknown version \"" << version << "\"\n";
    return false;
}

int main(int argn, char** args)
{
    if (argn < 3) {
        std::cout << "Invalid number of arguments -- please provide a source path and destination path" << '\n';
        system("pause");
        return 0;
    }

    char* sourcePath = args[1];
    char* destPath = args[2];

    std::ifstream sourceFile(sourcePath);

    if (!sourceFile.is_open()) {
        char errorMsg[64];
        errorMsg[63] = '\0';
        strerror_s(errorMsg, errno);

        std::cout << "Error opening source file: " << errorMsg;
        return 0;
    }

    std::ofstream destFile(destPath, std::ios::trunc);

    if (!destFile.is_open()) {
        std::cout << "Error opening/creating destination file";
        return 0;
    }

    if (convert(sourcePath, sourceFile, destFile)) std::cout << "Done";

    sourceFile.close();
    destFile.close();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
