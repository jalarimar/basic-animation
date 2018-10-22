//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2018)
//
//  FILE NAME: task2.cpp
//  Uses the Assimp and DevIL libraries.   
//  
//  ========================================================================

#include <iostream>
#include <map>
#include <GL/freeglut.h>
#include <IL/il.h>
using namespace std;

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "assimp_extras.h"

const aiScene* scene = NULL;
GLuint scene_list = 0;
float angle = 0;
aiVector3D scene_min, scene_max;
bool modelRotn = true;
std::map<int, int> texIdMap;

aiVector3D rootPos;

typedef struct original_mesh {
	int nVerts;
	aiVector3D* verts;
	aiVector3D* norms;
} OriginalMesh;

OriginalMesh* all_meshes;


bool loadModel(const char* fileName)
{
	scene = aiImportFile(fileName, aiProcessPreset_TargetRealtime_MaxQuality);
	if(scene == NULL) exit(1);
	// printSceneInfo(scene);
	// printTreeInfo(scene->mRootNode);
	// printBoneInfo(scene);
	// printAnimInfo(scene);
	get_bounding_box(scene, &scene_min, &scene_max);
	return true;
}

void loadGLTextures(const aiScene* scene)
{

	/* initialization of DevIL */
	ilInit();
	if (scene->HasTextures())
	{
		std::cout << "Support for meshes with embedded textures is not implemented" << endl;
		exit(1);
	}

	/* scan scene's materials for textures */
	/* Simplified version: Retrieves only the first texture with index 0 if present*/
	for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
	{
		aiString path;  // filename

		// if (scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
		// {
			path = "./fur.jpeg";
			glEnable(GL_TEXTURE_2D);
			ILuint imageId;
			GLuint texId;
			ilGenImages(1, &imageId);
			glGenTextures(1, &texId);
			texIdMap[m] = texId;   //store tex ID against material id in a hash map
			ilBindImage(imageId); /* Binding of DevIL image name */
			ilEnable(IL_ORIGIN_SET);
			ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
			if (ilLoadImage((ILstring)path.data))   //if success
			{
				/* Convert image to RGBA */
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

				/* Create and load textures to OpenGL */
				glBindTexture(GL_TEXTURE_2D, texId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
					ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE,
					ilGetData());
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				cout << "Texture:" << path.data << " successfully loaded." << endl;
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			}
			else
			{
				cout << "Couldn't load Image: %s\n" << path.data << endl;
			}
		// } else {
		// 	cout << "load failed" << endl;
		// }
	}  //loop for material
}


// ------A recursive function to traverse scene graph and render each mesh----------
void render (const aiScene* sc, const aiNode* nd)
{
	aiMatrix4x4 m = nd->mTransformation;
	aiMesh* mesh;
	aiFace* face;
	GLuint texId;
	int meshIndex, materialIndex;

	aiTransposeMatrix4(&m);   //Convert to column-major order
	glPushMatrix();
	glMultMatrixf((float*)&m);   //Multiply by the transformation matrix for this node

	// Draw all meshes assigned to this node
	for (int n = 0; n < nd->mNumMeshes; n++)
	{
		meshIndex = nd->mMeshes[n];          //Get the mesh indices from the current node
		mesh = sc->mMeshes[meshIndex];    //Using mesh index, get the mesh object
		apply_material(sc->mMaterials[mesh->mMaterialIndex]);  //Change opengl state to that material's properties 

		if(mesh->HasNormals())
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);

		if (mesh->HasTextureCoords(0)) {
			glEnable(GL_TEXTURE_2D);
			int texId = texIdMap[meshIndex];
			glBindTexture(GL_TEXTURE_2D, texId);
		}

		//Get the polygons from each mesh and draw them
		for (int k = 0; k < mesh->mNumFaces; k++)
		{
			face = &mesh->mFaces[k];
			GLenum face_mode;

			switch(face->mNumIndices)
			{
				case 1: face_mode = GL_POINTS; break;
				case 2: face_mode = GL_LINES; break;
				case 3: face_mode = GL_TRIANGLES; break;
				default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for(int i = 0; i < face->mNumIndices; i++) {
				int vertexIndex = face->mIndices[i]; 

				if(mesh->HasVertexColors(0))
					glColor4fv((GLfloat*)&mesh->mColors[0][vertexIndex]);

				if (mesh->HasNormals())
					glNormal3fv(&mesh->mNormals[vertexIndex].x);

				glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x,
							mesh->mTextureCoords[0][vertexIndex].y);
				glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}

			glEnd();
		}

	}
	// Draw all children
	for (int i = 0; i < nd->mNumChildren; i++)
		render(sc, nd->mChildren[i]);

	glPopMatrix();
}

//--------------------OpenGL initialization------------------------
void initialise()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	loadModel("wuson.x");			//<<<-------------Specify input file name here
	loadGLTextures(scene);        //<<<-------------Uncomment when implementing texturing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 1.0, 1000.0);

	all_meshes = (OriginalMesh*)malloc(sizeof(OriginalMesh) * scene->mNumMeshes);
	
	for (int i=0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		aiVector3D* verts = (aiVector3D*)malloc(sizeof(aiVector3D) * mesh->mNumVertices);
		aiVector3D* norms = (aiVector3D*)malloc(sizeof(aiVector3D) * mesh->mNumVertices);

		for (int j=0; j < mesh->mNumVertices; j++) {
			verts[j] = mesh->mVertices[j];
			norms[j] = mesh->mNormals[j];
		}
		all_meshes[i].nVerts = mesh->mNumVertices;
		all_meshes[i].verts = verts;
		all_meshes[i].norms = norms;
	}
}

aiVector3D interpolate_position(aiNodeAnim* channel, int tick) {
	int index;
	for (int i=0; i < channel->mNumPositionKeys; i++) {
		if (tick < channel->mPositionKeys[i].mTime) {
			index = i;
			break;
		}
	}

	aiVector3D startPos = (channel->mPositionKeys[index-1]).mValue;
	aiVector3D endPos = (channel->mPositionKeys[index]).mValue;
	double time1 = (channel->mPositionKeys[index-1]).mTime;
	double time2 = (channel->mPositionKeys[index]).mTime;
	float factor = (tick-time1)/(time2-time1);
	aiVector3D out = startPos + factor * (endPos - startPos);
	return out;
}

aiQuaternion interpolate_rotn(aiNodeAnim* channel, int tick) {
	int index;
	for (int i=0; i < channel->mNumRotationKeys; i++) {
		if (tick < channel->mRotationKeys[i].mTime) {
			index = i;
			break;
		}
	}

	aiQuaternion rotn1 = (channel->mRotationKeys[index-1]).mValue;
	aiQuaternion rotn2 = (channel->mRotationKeys[index]).mValue;
	double time1 = (channel->mRotationKeys[index-1]).mTime;
	double time2 = (channel->mRotationKeys[index]).mTime;
	double factor = (tick-time1)/(time2-time1);
	aiQuaternion out;
	aiQuaternion::Interpolate(out, rotn1, rotn2, factor);
	return out.Normalize();
}

//----Timer callback ----
void update(int time)
{
	aiAnimation* anim = scene->mAnimations[0]; // todo wuson has 2 animations in the file - key to toggle
	double ticksPerSec = anim->mTicksPerSecond;
	int tick = (time * ticksPerSec) / 1000; // 48 to inf, incr by 48, ticksPerSec = 4800
	tick = fmod(tick, anim->mDuration);
	
	if (tick < anim->mDuration) { // 4640 divide 160 time between keys = 29 keys + 1 startpos
		for (int i = 0; i < anim->mNumChannels; i++) {
			aiNodeAnim* channel = anim->mChannels[i];
			//cout << channel->mNodeName.C_Str() << " : " << channel->mNumRotationKeys << endl;

			aiVector3D posn = channel->mPositionKeys[0].mValue;
			if (channel->mNumPositionKeys > 1) { // 30 for skeleton, 1 for others
				posn = interpolate_position(channel, tick);
				rootPos = posn;
			}
			// mNumRotationKeys either 1, 28, 29, 30
			aiQuaternion rotn = channel->mRotationKeys[0].mValue; // should be 0
			if (channel->mNumRotationKeys > 1) {
				rotn = interpolate_rotn(channel, tick);
			}

			aiMatrix4x4 matPos;
			matPos.Translation(posn, matPos);
			aiMatrix4x4 matRot = aiMatrix4x4(rotn.GetMatrix());
			aiMatrix4x4 prod = matPos * matRot;

			aiNode* node = scene->mRootNode->FindNode(channel->mNodeName);
			node->mTransformation = prod;
		}
	}

	for (int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		for (int v; v < mesh->mNumVertices; v++) {
			mesh->mVertices[v] = aiVector3D(0);
			mesh->mNormals[v] = aiVector3D(0);
		}

		for (int j = 0; j < mesh->mNumBones; j++) {
			aiBone* bone = mesh->mBones[j];
			aiMatrix4x4 offset = bone->mOffsetMatrix;

			aiNode* node = scene->mRootNode->FindNode(bone->mName);
			aiMatrix4x4 product = node->mTransformation * offset;

			while (node->mParent) {
				aiNode* parent = node->mParent;
				aiMatrix4x4 transformation = parent->mTransformation;
				product = transformation * product;
				node = parent;
			}

			// aiMatrix4x4 p = product;
			// cout << " " << p.a1 << " " << p.a2 << " " << p.a3 << " " << p.a4 << " " << endl;
			// cout << " " << p.b1 << " " << p.b2 << " " << p.b3 << " " << p.b4 << " " << endl;
			// cout << " " << p.c1 << " " << p.c2 << " " << p.c3 << " " << p.c4 << " " << endl;
			// cout << " " << p.d1 << " " << p.d2 << " " << p.d3 << " " << p.d4 << " " << endl;
			aiMatrix4x4 transposeProduct = product;
			transposeProduct.Transpose();	

			for (int k = 0; k < bone->mNumWeights; k++) {
				int vertexId = bone->mWeights[k].mVertexId;
				float weight = bone->mWeights[k].mWeight;

				OriginalMesh orig = all_meshes[i];
				aiVector3D position = orig.verts[vertexId];
				aiVector3D normal = orig.norms[vertexId];
				
				mesh->mVertices[vertexId] += weight * (product * position);
				mesh->mNormals[vertexId] += weight * (transposeProduct * normal);
			}
		}
	}

	glutPostRedisplay();
	glutTimerFunc(10, update, time + 10);
}

//----Keyboard callback to toggle initial model orientation---
void keyboard(unsigned char key, int x, int y)
{
	if(key == '1') modelRotn = !modelRotn;   //Enable/disable initial model rotation
	glutPostRedisplay();
}

//------The main display function---------
//----The model is first drawn using a display list so that all GL commands are
//    stored for subsequent display updates.
void display()
{
	float pos[4] = {50, 50, 50, 1};
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	// scale the whole asset to fit into our view frustum 
	float tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

	float rx = rootPos.x; 
	float ry = rootPos.y;
	float rz = rootPos.z;

	gluLookAt(-3, 3, 7, rx, ry, rz, 0, 1, 0);

	// floor
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.8, 0.72, 0.6, 1.0); 
	glNormal3f(0.0, 1.0, 0.0);

	glBegin(GL_QUADS);
		int xmin = -100;
		int xmax = 150;
		int zmin = -100;
		int zmax = 150;
		int y = -5;
		glVertex3f(xmin, y, zmin);
		glVertex3f(xmin, y, zmax);
		glVertex3f(xmax, y, zmax);
		glVertex3f(xmax, y, zmin);
	glEnd();

	glColor4f(0.4, 0.4, 0.4, 1.0);
	
	glRotatef(90, 1, 0, 0);
	glRotatef(90, 0, 0, 1);
	render(scene, scene->mRootNode);
	
	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Task 3");
	glutInitContextVersion (4, 2);
	glutInitContextProfile ( GLUT_CORE_PROFILE );

	initialise();
	glutDisplayFunc(display);
	glutTimerFunc(10, update, 10);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	aiReleaseImport(scene);
}

