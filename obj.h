#ifndef __OBJ_H__
#define __OBJ_H__

#include <GLES2/gl2.h>
#include <cstdio>
#include <cstring>
#include <vector>

typedef struct
{
  float x;
  float y;
  float z;
} vec3;

typedef struct
{
  float x;
  float y;
} vec2;

class OBJObject
{
public:
    OBJObject(void)
    {
        raw = NULL;
	count = 0;
    }

    virtual ~OBJObject(void)
    {
        Free();
    }
   
    bool LoadFromOBJ(const char * filename, float scale=1.0f)
    {
        FILE * f = NULL;

        f = fopen(filename, "r");
        if(f == NULL)
        {
            return false;
        }

	std::vector <unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector <vec3> temp_vertices;
	std::vector <vec2> temp_uvs;
	std::vector <vec3> temp_normals;

	std::vector <vec3> out_vertices;
	std::vector <vec2> out_uvs;
	std::vector <vec3> out_normals;


	while (1) {
		char lineHeader[128];

		int res = fscanf(f, "%s", lineHeader);
		if (res == EOF)
			break;

		if (strcmp(lineHeader, "v")==0) {
			vec3 vertex;
			fscanf(f, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertex.x*=scale;
			vertex.y*=scale;
			vertex.z*=scale;
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt")==0) {
			vec2 uv;
			fscanf(f, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
			//fprintf(stderr, "uv = %f, %f\n", uv.x, uv.y);
		}
		else if (strcmp(lineHeader, "vn")==0) {
			vec3 normal;
			fscanf(f, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f")==0) {
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches  = fscanf(f, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
				&vertexIndex[0], &uvIndex[0], &normalIndex[0],
				&vertexIndex[1], &uvIndex[1], &normalIndex[1],
				&vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("file can't be read by our simple parser\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);	
			vertexIndices.push_back(vertexIndex[2]);	
			uvIndices.push_back(uvIndex[0]);	
			uvIndices.push_back(uvIndex[1]);	
			uvIndices.push_back(uvIndex[2]);	
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);

		}
	}
	fclose(f);

	for (unsigned int i=0; i<vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		vec3 vertex = temp_vertices[vertexIndex-1];
		out_vertices.push_back(vertex);
	}
	for (unsigned int i=0; i<uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		vec2 uv = temp_uvs[uvIndex-1];
		out_uvs.push_back(uv);
	}
	for (unsigned int i=0; i<normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		vec3 normal = temp_normals[normalIndex-1];
		out_normals.push_back(normal);
	}
	//fprintf(stderr, "vertices:%d, uvs:%d, normals: %d\n", out_vertices.size(),
	//						      out_uvs.size(),
	//					              out_normals.size());
	count = out_vertices.size();

	raw = (unsigned char *)malloc(count * sizeof(float)*(4+3+2));
	float *p = (float *)raw;
	for (int x=0; x<count; x++) {
		vec3 v = out_vertices[x];
		*p = v.x; p++;
		*p = v.y; p++;
		*p = v.z; p++;
		*p = 1.0f; p++;
	}
	for (int x=0; x<count; x++) {
		vec3 n = out_normals[x];
		*p = n.x; p++;
		*p = n.y; p++;
		*p = n.z; p++;
	}
	for (int x=0; x<count; x++) {
		vec2 uv = out_uvs[x];
		*p = uv.x; p++;
		*p = uv.y; p++;
		//fprintf(stderr, "uv= %f, %f\n", uv.x, uv.y);
	}
        return true;
    }

    bool Free(void)
    {
	    free(raw);
	    raw = NULL;
	    count = 0;
	    return true;
    }


    unsigned int GetAttributeCount(void) const
    {
        return count;
    }

    unsigned int GetAttribComponents(unsigned int index) const
    {
        switch (index) {
		case 0:
			return 4;
		case 1:
			return 3;
		case 2:
			return 2;
	}
    }

    unsigned char* GetVertexData()
    {
        return raw;
    }

    unsigned int GetNumVertices() const
    {
        return count;
    }

    unsigned int GetFirstFrameVertex(unsigned int frame) const
    {
        return 0;
    }

    unsigned int GetFrameVertexCount(unsigned int frame) const
    {
        return count;
    }

protected:
	int count;
	unsigned char *raw;
   };

#endif /* __OBJ_H__ */
