// ----------------------------------------------------------------------------
// Simple sample to prove that Assimp is easy to use with OpenGL.
// It takes a file name as command line parameter, loads it using standard
// settings and displays it.
//
// If you intend to _use_ this code sample in your app, do yourself a favour 
// and replace immediate mode calls with VBOs ...
//
// The vc8 solution links against assimp-release-dll_win32 - be sure to
// have this configuration built.
// ----------------------------------------------------------------------------


// assimp include files. These three are usually needed.
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"

#include "GL/glut.h"
#include <IL/il.h>

//to map image filenames to textureIds
#include <string.h>
#include <map>

//math functions
#include <cmath>


#include <cstdlib>

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


// currently this is hardcoded
static const std::string basepath = "./models/"; //per i file blend

// the global Assimp scene object
const struct aiScene* scene = NULL;
GLuint scene_list = 0;
struct aiVector3D scene_min, scene_max, scene_center;

// current rotation angle
static float angle = 0.f;

// images / texture
std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds
GLuint*		textureIds;							// pointer to texture Array

GLfloat LightAmbient[] = { 0.01f, 0.015f, 0.015f, 1.0f };
GLfloat LightDiffuse[]= { 0.6f, 0.6f,0.6f, 1.0f };
GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f };

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)
#define TRUE                1
#define FALSE               0
// definizioni di variabili di gameplay
bool menu = true;
bool credits = false;
bool invincible = false;
bool salto = false;
bool discesa = false;
bool staccionatatime = false;
bool scoremenu = false;
bool passaononpassa = false;
bool showMenu = false;
float vitachepassa = 10;
float x_coord_t = 0.0;
float x_coord_obs, z_coord_obs, z_coord_bck= -50;
float z_coord_stac= -40;
float z_coord_bck2 = -185;
int casuale = 0;
int window_height = 100;
int window_width = 1000;
int vite = 3;
int durata = 0;
int score = 0;
int casual = 0;
float y_salto = 0;

int attesa = 0;

// ----------------------------------------------------------------------------
void reshape(int width, int height)
{
	const double aspectRatio = (float) width / height, fieldOfView = 45.0;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldOfView, aspectRatio,
		1.0, 1000.0);  // Znear and Zfar 
	glViewport(0, 0, width, height);
}
//-----------------------------------------------------------------------------
void draw_text(const char* format, int x, int y)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, window_width, 0, window_height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(x, y, 0);

	glRasterPos2i(0, 0);
	for (int i = 0; i < strlen(buffer); i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, buffer[i]);
	}

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
// ----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void get_bounding_box_for_node (const struct aiNode* nd, 
	struct aiVector3D* min, 
	struct aiVector3D* max, 
	struct aiMatrix4x4* trafo
){
	struct aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo,&nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			struct aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp,trafo);

			min->x = aisgl_min(min->x,tmp.x);
			min->y = aisgl_min(min->y,tmp.y);
			min->z = aisgl_min(min->z,tmp.z);

			max->x = aisgl_max(max->x,tmp.x);
			max->y = aisgl_max(max->y,tmp.y);
			max->z = aisgl_max(max->z,tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n],min,max,trafo);
	}
	*trafo = prev;
}

// ----------------------------------------------------------------------------

void get_bounding_box (struct aiVector3D* min, struct aiVector3D* max)
{
	struct aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene->mRootNode,min,max,&trafo);
}

// ----------------------------------------------------------------------------

void color4_to_float4(const struct aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

// ----------------------------------------------------------------------------

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

// ----------------------------------------------------------------------------
void apply_material(const struct aiMaterial *mtl)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	struct aiColor4D diffuse;
	struct aiColor4D specular;
	struct aiColor4D ambient;
	struct aiColor4D emission;
	float shininess, strength;
	int two_sided;
	int wireframe;
	int max;

int texIndex = 0;
aiString texPath;	//contains filename of texture
if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
{
	//bind texture
	unsigned int texId = *textureIdMap[texPath.data];
	glBindTexture(GL_TEXTURE_2D, texId);
}

	set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		color4_to_float4(&diffuse, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
		color4_to_float4(&specular, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

	set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
		color4_to_float4(&ambient, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
		color4_to_float4(&emission, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

	max = 1;
	ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, (unsigned int *)&max);
	max = 1;
	ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, (unsigned int *)&max);
	if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
	else {
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
	}

	max = 1;
	if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, (unsigned int *)&max))
		fill_mode = wireframe ? GL_LINE : GL_FILL;
	else
		fill_mode = GL_FILL;
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	max = 1;
	if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, (unsigned int *)&max)) && two_sided)
		glEnable(GL_CULL_FACE);
	else 
		glDisable(GL_CULL_FACE);
}

// ----------------------------------------------------------------------------

// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
void Color4f(const struct aiColor4D *color)
{
	glColor4f(color->r, color->g, color->b, color->a);
}


// ----------------------------------------------------------------------------

void recursive_render (const struct aiScene *sc, const struct aiNode* nd, float scale)
{
	unsigned int i;
	unsigned int n=0, t;
	struct aiMatrix4x4 m = nd->mTransformation;

	//printf("Node name: %s\n",nd->mName.data);	

	
	// update transform
	m.Transpose();
	glPushMatrix();
	//glTranslatef(x_coord_t, 0, 0);
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		//printf("Drawing MESH with this name: %s\n",mesh->mName.data);

		apply_material(sc->mMaterials[mesh->mMaterialIndex]);


		if(mesh->HasTextureCoords(0))
			glEnable(GL_TEXTURE_2D);
		else
			glDisable(GL_TEXTURE_2D);
		if(mesh->mNormals == NULL)
		{
			glDisable(GL_LIGHTING);
		}
		else
		{
			glEnable(GL_LIGHTING);
		}

		if(mesh->mColors[0] != NULL)
		{
			glEnable(GL_COLOR_MATERIAL);
		}
		else
		{
			glDisable(GL_COLOR_MATERIAL);
		}



		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch(face->mNumIndices)
			{
				case 1: face_mode = GL_POINTS; break;
				case 2: face_mode = GL_LINES; break;
				case 3: face_mode = GL_TRIANGLES; break;
				default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for(i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				int vertexIndex = face->mIndices[i];	// get group index for current index
				if(mesh->mColors[0] != NULL)
					Color4f(&mesh->mColors[0][vertexIndex]);
				if(mesh->mNormals != NULL)

					if(mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
					{
						glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
					}

					glNormal3fv(&mesh->mNormals[vertexIndex].x);
					glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}

			glEnd();

		}

	}


	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, nd->mChildren[n], scale);
	}

	glPopMatrix();
}

// ----------------------------------------------------------------------------
void do_motion (void)
{
	static GLint prev_time = 0;
	int time = glutGet(GLUT_ELAPSED_TIME);
	if (!menu) {
		// Update coordinates
		if (score < 300) {
			z_coord_obs += (time - prev_time) * 0.02;
			z_coord_bck += (time - prev_time) * 0.02;
			z_coord_bck2 += (time - prev_time) * 0.02;
			if (staccionatatime == true)
				z_coord_stac += (time - prev_time) * 0.02;
		}
		else if (score < 800) {
			z_coord_obs += (time - prev_time) * 0.035;
			z_coord_bck += (time - prev_time) * 0.035;
			z_coord_bck2 += (time - prev_time) * 0.035;
			if (staccionatatime == true)
				z_coord_stac += (time - prev_time) * 0.035;
		}
		else if (score < 1300) {
			z_coord_obs += (time - prev_time) * 0.05;
			z_coord_bck += (time - prev_time) * 0.05;
			z_coord_bck2 += (time - prev_time) * 0.05;
			if (staccionatatime == true)
				z_coord_stac += (time - prev_time) * 0.05;
		}
		else if (score < 2000) {
			z_coord_obs += (time - prev_time) * 0.06;
			z_coord_bck += (time - prev_time) * 0.06;
			z_coord_bck2 += (time - prev_time) * 0.06;
			if (staccionatatime == true)
				z_coord_stac += (time - prev_time) * 0.06;
		}
		else {
			z_coord_obs += (time - prev_time) * 0.06;
			z_coord_bck += (time - prev_time) * 0.06;
			z_coord_bck2 += (time - prev_time) * 0.06;
			if (staccionatatime == true)
				z_coord_stac += (time - prev_time) * 0.06;
		}

		if (z_coord_obs > 10) {			// Reset position for the obstacles
			z_coord_obs = -55;

			srand(time);
			// Get a random number for the x_coord

			casuale = (rand() % (7 - 1)) + 1; // (rand() % (max- min)) + min , genera numero casuale tra max e min;
			if (casuale == 6 || casuale == 5 || casuale == 7)
				x_coord_obs = 2.0;
			if (casuale == 4 || casuale == 3)
				x_coord_obs = 0.0;
			if (casuale == 2 || casuale == 1)
				x_coord_obs = -2.0;
		}
		
		if (z_coord_bck > 130)			// Reset position for the background
		{
			z_coord_bck = -130;
		}
		if (z_coord_bck2 > 130)			// Reset position for the background
		{
			z_coord_bck2 = -130;
		}

		if (z_coord_stac > 10) {
			z_coord_stac = -50;
			staccionatatime = false;
		}

		prev_time = time;

		glutPostRedisplay();
	}
}

void check_collisions()
{
	if (z_coord_obs > -2 && z_coord_obs < 2) {
		if (abs(x_coord_t - x_coord_obs) < 2 && invincible == false) {
			invincible = true;
			vite--;
			if (vite == 0){
				 menu = true;
				 scoremenu = true;
				 showMenu = false;
				 x_coord_t = 0.0;
				 x_coord_obs, z_coord_obs, z_coord_bck = -50;
				 z_coord_stac = -40;
			     z_coord_bck2 = -185;
				 casuale = 0;
				 window_height = 900;
				 window_width = 600;
				 vite = 3;
				 durata = 0;
				 invincible = false;
				 salto = false;
				 y_salto = 0;
				 discesa = false;
				 attesa = 0;
				 staccionatatime = false;
			}
		}
	}
	if (z_coord_stac > -2 && z_coord_stac < 2) {
		if (y_salto < 1.2 && invincible == false) {
			invincible = true;
			vite--;
			if (vite == 0){
				 menu = true;
				 scoremenu = true;
				 showMenu = false;
				 x_coord_t = 0.0;
				 x_coord_obs, z_coord_obs, z_coord_bck = -50;
				 z_coord_stac = -40;
			     z_coord_bck2 = -185;
				 casuale = 0;
				 window_height = 900;
				 window_width = 600;
				 vite = 3;
				 durata = 0;
				 invincible = false;
				 salto = false;
				 y_salto = 0;
				 discesa = false;
				 attesa = 0;
				 staccionatatime = false;
			}
		}
	}
}
// ----------------------------------------------------------------------------
void display(void) {
	float tmp;
	if (!showMenu) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		if (!menu) {
			gluLookAt(0.f, 3.f, 7.f, 0.f, 0.f, -5.f, 0.f, 1.f, 0.f);

			if (invincible == true) {
				durata++;
				if (durata > 90) {
					invincible = false;
					durata = 0;
				}
			}
			score++;

			//draw tumbleweed
			if (salto == true) {
				if (discesa == false) {
					if (attesa == 0)
						y_salto += 0.3;
					if (y_salto > 2) {
						attesa++;
						if (attesa == 12) {
							discesa = true;
							attesa = 0;
						}
					}
				}
				else if (discesa == true) {
					y_salto -= 0.3;
					if (y_salto <= 0) {
						discesa = false;
						salto = false;
					}
				}

			}

			if (durata % 2 == 0) {
				glPushMatrix();
				glTranslatef(x_coord_t, y_salto + 0.8, 0);
				glRotatef(z_coord_bck * 10, -0.5, 0, 0);
				recursive_render(scene, scene->mRootNode->mChildren[3], 1.0);
				glPopMatrix();
			}

			//draw cactus
			glPushMatrix();
			glTranslatef(x_coord_obs, 0, z_coord_obs);
			recursive_render(scene, scene->mRootNode->mChildren[0], 1.0);
			glPopMatrix();

			//draw ground1

			glPushMatrix();
			glTranslatef(0, 0, z_coord_bck);
			recursive_render(scene, scene->mRootNode->mChildren[6], 1.0);
			glPopMatrix();

			//draw ground2

			glPushMatrix();
			glTranslatef(0, 0, z_coord_bck2);
			recursive_render(scene, scene->mRootNode->mChildren[5], 1.0);
			glPopMatrix();

			//draw sky1

			glPushMatrix();
			glTranslatef(z_coord_bck, 0, 0);
			recursive_render(scene, scene->mRootNode->mChildren[4], 1.0);
			glPopMatrix();

			//draw sky2

			glPushMatrix();
			glTranslatef(z_coord_bck2, 0, 0);
			recursive_render(scene, scene->mRootNode->mChildren[7], 1.0);
			glPopMatrix();

			//draw spine
			glPushMatrix();
			glTranslatef(x_coord_obs, 0, z_coord_obs);
			recursive_render(scene, scene->mRootNode->mChildren[1], 1.0);
			glPopMatrix();

			//draw vite rimaste - cuori
			if (vite> 0) {
				glPushMatrix();
				glTranslatef(-10, 4, -20);
				recursive_render(scene, scene->mRootNode->mChildren[10],0.5);
				glPopMatrix();
				if (vite > 1) {
					glPushMatrix();
					glTranslatef(-8, 4, -20);
					recursive_render(scene, scene->mRootNode->mChildren[10], 0.5);
					glPopMatrix();
				}
				if (vite > 2) {
					glPushMatrix();
					glTranslatef(-6, 4, -20);
					recursive_render(scene, scene->mRootNode->mChildren[10], 0.5);
					glPopMatrix();
				}
			}

			//draw vita che passa ogni tanto 
			/*
			if (score % 1000 == 0) { 
				passaononpassa = true;
				vitachepassa = 10;
				casual = (rand() % (25)) -5;
			} 
			if(passaononpassa==true) {
			glPushMatrix();
			glTranslatef(casual, vitachepassa, -12);
			recursive_render(scene, scene->mRootNode->mChildren[12], 1);
			glPopMatrix();
			vitachepassa -= 0.025;
			if (vitachepassa == 8){
				passaononpassa = false;
				vitachepassa = 10;
	
			}
		}*/
			//draw staccionata
			if (score % 150 == 0) staccionatatime = true;
			if (staccionatatime == true) {
				glPushMatrix();
				glTranslatef(0, 0, z_coord_stac);
				recursive_render(scene, scene->mRootNode->mChildren[2], 1.0);
				glPopMatrix();
			}




			//draw punteggio
			char score_str[21];
			sprintf(score_str, "Punteggio: %d", score);
			draw_text(score_str, 30 , window_height - 3.5);

			//draw passavita
			/*
			char vita_str[30];
			sprintf(vita_str, "altvita: %d", vitachepassa);
			glColor3f(1.0f, 1.0f, 1.0f);
			draw_text(vita_str, 10, window_height - 30);
			
			char terreno_str[30];
			sprintf(terreno_str, "terreno1: %f.4", z_coord_bck);
			glColor3f(1.0f, 1.0f, 1.0f);
			draw_text(terreno_str, 10, window_height - 80);
			char terreno2_str[30];
			sprintf(terreno2_str, "terreno2: %f.4", z_coord_bck2);
			glColor3f(1.0f, 1.0f, 1.0f);
			draw_text(terreno2_str, 10, window_height - 100);
			
			//draw cuore rosso per aumentare le vite
			glBegin(GL_POLYGON);
			glColor3f(1.0f, 1.0f, 1.0f);
			// Left half of the heart
			glVertex2f(-0.5, 0.0);
			glVertex2f(-0.8, 0.5);
			glVertex2f(-0.8, 0.8);
			glVertex2f(-0.5, 1.0);
			glVertex2f(0.0, 0.8);
			// Right half of the heart
			glVertex2f(0.5, 1.0);
			glVertex2f(0.8, 0.8);
			glVertex2f(0.8, 0.5);
			glVertex2f(0.5, 0.0);
			// Bottom point of the heart
			glVertex2f(0.0, -0.5);
			glEnd();
			*/

			//glColor3f(1.0, 0.0, 0.0); // impostare il colore del testo a rosso
			//draw_text("Il mio testo rosso", 100, 100);
			/*
			//draw tiers
			glPushMatrix();
				glTranslatef(x_coord_t, -0.3, -1.5);
				glRotatef(z_coord_bck * -10, 1, 0, 0);
				recursive_render(scene, scene->mRootNode->mChildren[3], 1.0);
			glPopMatrix();

			glPushMatrix();
				glTranslatef(x_coord_t, -0.3, 1.5);
				glRotatef(z_coord_bck * -10, 1, 0, 0);
				recursive_render(scene, scene->mRootNode->mChildren[4], 1.0);
			glPopMatrix();
			*/
			
			glutSwapBuffers();
			do_motion();
			check_collisions();
		}
		else {

		gluLookAt(0.f, 3.f, 0.f, 0.f, 0.f, -50.f, 0.f, 1.f, 0.f);
		//drawMenu
		glPushMatrix();
		glTranslatef(0, -5, -10);
		if (credits == false && scoremenu== false ){
			recursive_render(scene, scene->mRootNode->mChildren[9], 1.0);
		}
		else if(scoremenu==true) {
			recursive_render(scene, scene->mRootNode->mChildren[11], 1.0);
			//draw punteggio
			char score2_str[10];
			sprintf(score2_str, "%d", score);
			//draw_text(score2_str, 330, window_height - 120);
				int numbers[10];
				float pos_x = 0;//TODO
				int i = 0;
				while (score != 0) {
					numbers[i] = score % 10; // ottiene l'ultima cifra del numero
					glPushMatrix();
					glTranslatef(pos_x, 0, 0);
					switch (numbers[i]) {
					case 1: recursive_render(scene, scene->mRootNode->mChildren[13], 1.0); break;
					case 2: recursive_render(scene, scene->mRootNode->mChildren[14], 1.0); break;
					case 3: recursive_render(scene, scene->mRootNode->mChildren[15], 1.0); break;
					case 4: recursive_render(scene, scene->mRootNode->mChildren[16], 1.0); break;
					case 5: recursive_render(scene, scene->mRootNode->mChildren[17], 1.0); break;
					case 6: recursive_render(scene, scene->mRootNode->mChildren[18], 1.0); break;
					case 7: recursive_render(scene, scene->mRootNode->mChildren[19], 1.0); break;
					case 8: recursive_render(scene, scene->mRootNode->mChildren[20], 1.0); break;
					case 9: recursive_render(scene, scene->mRootNode->mChildren[21], 1.0); break;
					default: recursive_render(scene, scene->mRootNode->mChildren[12], 1.0);
					}
					i++;
					score = score / 10; // rimuove l'ultima cifra dal numero
					pos_x -= 0.38;
					glPopMatrix();
				}
				
			
		}
		else {
			recursive_render(scene, scene->mRootNode->mChildren[8], 1.0);
		}
		glPopMatrix();
		
		glutSwapBuffers();


	}
	}
	
}

// ----------------------------------------------------------------------------

void specialKeyListener(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_RIGHT:
		if (x_coord_t <= 0.0) {
			x_coord_t += 2.0;
		}
		break;
	case GLUT_KEY_LEFT:
	
		if (x_coord_t >= 0.0) {
			x_coord_t -= 2.0;
		}
		break;	
	case GLUT_KEY_UP:
		if (salto == false) {
			salto = true;
		}
		break;
	default:
		break;

	}
	glutPostRedisplay();
}

int loadasset (const char* path)
{
	// we are taking one of the postprocessing presets to avoid
	// writing 20 single postprocessing flags here.
	scene = aiImportFile(path,aiProcessPreset_TargetRealtime_Quality);

	if (scene) {
		get_bounding_box(&scene_min,&scene_max);
		scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
		return 0;
	}
	return 1;
}

int LoadGLTextures(const aiScene* scene)
{
	ILboolean success;

	/* Before calling ilInit() version should be checked. */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		ILint test = ilGetInteger(IL_VERSION_NUM);
		/// wrong DevIL version ///
		std::string err_msg = "Wrong DevIL version. Old devil.dll in system32/SysWow64?";
		char* cErr_msg = (char *) err_msg.c_str();
		
		return -1;
	}

	ilInit(); /* Initialization of DevIL */

	//if (scene->HasTextures()) abortGLInit("Support for meshes with embedded textures is not implemented");

	/* getTexture Filenames and Numb of Textures */
	for (unsigned int m=0; m<scene->mNumMaterials; m++)
	{
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;

		aiString path;	// filename

		while (texFound == AI_SUCCESS)
		{
			texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			textureIdMap[path.data] = NULL; //fill map with textures, pointers still NULL yet
			texIndex++;
		}
	}

	int numTextures = textureIdMap.size();

	/* array with DevIL image IDs */
	ILuint* imageIds = NULL;
	imageIds = new ILuint[numTextures];

	/* generate DevIL Image IDs */
	ilGenImages(numTextures, imageIds); /* Generation of numTextures image names */

	/* create and fill array with GL texture ids */
	textureIds = new GLuint[numTextures];
	glGenTextures(numTextures, textureIds); /* Texture name generation */

	/* define texture path */
	//std::string texturepath = "../../../test/models/Obj/";

	/* get iterator */
	std::map<std::string, GLuint*>::iterator itr = textureIdMap.begin();

	for (int i=0; i<numTextures; i++)
	{

		//save IL image ID
		std::string filename = (*itr).first;  // get filename
		(*itr).second =  &textureIds[i];	  // save texture id for filename in map
		itr++;								  // next texture


		ilBindImage(imageIds[i]); /* Binding of DevIL image name */
		std::string fileloc = basepath + filename;	/* Loading of image */
		success = ilLoadImage((const wchar_t *)fileloc.c_str());

		fprintf(stdout,"Loading Image: %s\n", fileloc.data());

		if (success) /* If no error occured: */
		{
			success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE); /* Convert every colour component into
			unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
			if (!success)
			{
				/* Error occured */
				fprintf(stderr,"Couldn't convert image");
				return -1;
			}
			//glGenTextures(numTextures, &textureIds[i]); /* Texture name generation */
			glBindTexture(GL_TEXTURE_2D, textureIds[i]); /* Binding of texture name */
			//redefine standard texture values
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear
			interpolation for magnification filter */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear
			interpolation for minifying filter */
			glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
				ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
				ilGetData()); /* Texture specification */
		}
		else
		{
			/* Error occured */			
			fprintf(stderr,"Couldn't load Image: %s\n", fileloc.data());
		}
	}
	ilDeleteImages(numTextures, imageIds); /* Because we have already copied image data into texture data
	we can release memory used by image. */

	//Cleanup
	delete [] imageIds;
	imageIds = NULL;

	//return success;
	return TRUE;
}

int InitGL()					 // All Setup For OpenGL goes here
{
		if (!LoadGLTextures(scene))
		{
			return FALSE;
		}

		glEnable(GL_TEXTURE_2D);
		glShadeModel(GL_SMOOTH);		 // Enables Smooth Shading
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);				// Depth Buffer Setup
		glEnable(GL_DEPTH_TEST);		// Enables Depth Testing
		glDepthFunc(GL_LEQUAL);			// The Type Of Depth Test To Do
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculation

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);    // Uses default lighting parameters
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		glEnable(GL_NORMALIZE);

		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
		glEnable(GL_LIGHT1);

		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		return TRUE;					// Initialization Went OK
	}


// ----------------------------------------------------------------------------
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
		GLint viewport[4];
		GLubyte pixel[3];

		// ottiene le coordinate del click e legge il colore del pixel
		glGetIntegerv(GL_VIEWPORT, viewport);
		glReadPixels(x, viewport[3] - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

		if (menu == true) {
			if (credits == false && scoremenu == false) {

				if ((y >= 90 && y <= 200) && (x >= 560 && x <= 980)) {
					menu = false;
					glutPostRedisplay();
				}
				if ((y >= 230 && y <= 340) && (x >= 560 && x <= 980)) {
					credits = true;
					glutPostRedisplay();
				}
				if ((y >= 375 && y <= 480) && (x >= 560 && x <= 980)) {
					exit(1);
				}
			}
			if (credits == true && scoremenu == false) {
				if ((y >= 450 && y <= 540) && (x >= 615 && x <= 905)) {
					credits = false;
					glutPostRedisplay();
				}
			}
			if (scoremenu){
					if ((y >= 230 && y <= 340) && (x >= 560 && x <= 980)) {
						menu = false;
						score = 0;
						x_coord_t = 0.0;
						x_coord_obs, z_coord_obs, z_coord_bck = -50;
						z_coord_stac = -40;
						z_coord_bck2 = -185;
						casuale = 0;
						window_height = 900;
						window_width = 600;
						vite = 3;
						durata = 0;
						invincible = false;
						salto = false;
						y_salto = 0;
						discesa = false;
						attesa = 0;
						staccionatatime = false;
						scoremenu = false;
						glutPostRedisplay();
					}
					if ((y >= 375 && y <= 480) && (x >= 560 && x <= 980)) {
						exit(1);
					}
				

		    }
		
			
		}
		
	}
}
// ----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	struct aiLogStream stream;
	
	

	
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInit(&argc, argv);

	glutCreateWindow("Tumbleweed game");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(specialKeyListener);
	glutMouseFunc(mouse);
	glutFullScreen();
	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It will be active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.

	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	aiAttachLogStream(&stream);

	// ... exactly the same, but this stream will now write the
	// log file to assimp_log.txt
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
	aiAttachLogStream(&stream);
	
	// the model name can be specified on the command line. 
	if(argc>=2)
		loadasset( argv[1] );
	else // otherwise the model is specified statically 
	{
		char* modelToLoad = "models\\tumbleweed_s.obj";
		fprintf(stdout, "loading: %s", modelToLoad);		
		loadasset(modelToLoad);
	}	
	
	
	

	if (!InitGL())
	{
		fprintf(stderr,"Initialization failed");
		return FALSE;
	}

	glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();

	// cleanup - calling 'aiReleaseImport' is important, as the library 
	// keeps internal resources until the scene is freed again. Not 
	// doing so can cause severe resource leaking.
	aiReleaseImport(scene);

	// We added a log stream to the library, it's our job to disable it
	// again. This will definitely release the last resources allocated
	// by Assimp.
	aiDetachAllLogStreams();	


	glutMainLoop();
	return 0; 

	
}
