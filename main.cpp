#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <thread>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "filesystem.h"
//#include "FrameBuffer.h"

using namespace std;

#define USE_WIREFRAME false

unsigned int TextureFromFile(const char* path, const string& directory);
struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

class Mesh {
	public:
		// Mesh Data
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);
		void Draw(Shader &shader);
	
	private:
		//Render data
		unsigned int VAO, VBO, EBO;
		void setupMesh();
};

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	setupMesh();
}
void Mesh::setupMesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	//Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

	glBindVertexArray(0);
}
void Mesh::Draw(Shader& shader)
{
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); // Activate texture unit first

		//retrieve texture number (the N in diffuse_textureN)
		string number;
		string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);

		shader.setFloat(("material." + name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);

	//Draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


class Model
{
	public:
		Model() {}
		Model(char* path)
		{
			loadModel(path);
		}
		void loadModel(string path);
		void Draw(Shader& shader);
	private:
		//Model data
		vector<Mesh> meshes;
		string directory;
		vector<Texture> textures_loaded;

		
		void processNode(aiNode *node, const aiScene *scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene);
		vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};
void Model::Draw(Shader& shader)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Draw(shader);
	}
}
void Model::loadModel(string path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "Error::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);
}
void Model::processNode(aiNode* node, const aiScene* scene)
{
	//Process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	//Process child nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		// Process vertex positions, normals, and texture coords
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		if (mesh->mTextureCoords[0]) //does the mesh contain texture coords?
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
		{
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}
		vertices.push_back(vertex);
	}

	//Process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	//Process materai
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}
	return Mesh(vertices, indices, textures);
}
vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
	vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			//Textures need to be loaded
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
		}
	}

	return textures;
}
unsigned int TextureFromFile(const char* path, const string& directory)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}


//Globals
bool mousePressed = false;
float yaw_alt = 0.0f;//can go up to 360deg
float pitch_alt = 0.0f;//-90 to 90 deg
float cam_radius = 200.0f;
static const float CAM_SPEED = 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
float lastX = 400, lastY = 300;
float pitch = 0.0f;
float yaw = -90.0f;
float fov = 45.0f;
bool firstMouse = true;

void framebuffser_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


void processInput(GLFWwindow* window)
{
	const float cameraSpeed = 20.0f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		lightPos.z += 0.5;
		pitch_alt += CAM_SPEED;
		if (pitch_alt > 85.0f)
			pitch_alt = 85.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		lightPos.z -= 0.5;
		pitch_alt -= CAM_SPEED;
		if (pitch_alt < -85.0f)
			pitch_alt = -85.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		lightPos.x += 0.5;
		yaw_alt -= CAM_SPEED;
		if (yaw_alt < 0.0f)
			yaw_alt += 360.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		lightPos.x -= 0.5;
		yaw_alt += CAM_SPEED;
		if (yaw_alt > 360.0f)
			yaw_alt -= 360.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
	{
		lightPos.y += 0.5;
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
	{
		lightPos.y -= 0.5;
	}
	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
	{
		printf("Light pos: (%f, %f, %f)\n", lightPos.x, lightPos.y, lightPos.z);
	}
	
}




void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}
	else if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	glm::vec3 front;
	float yaw_rads = glm::radians(yaw);
	float pitch_rads = glm::radians(pitch);
	front.x = cos(yaw_rads) * cos(pitch_rads);
	front.y = sin(pitch_rads);
	front.z = sin(yaw_rads) * cos(pitch_rads);
	cameraFront = glm::normalize(front);

	//printf("xoffset = %f, yoffset = %f\n", xoffset, yoffset);
	if (mousePressed) 
	{
		yaw_alt -= xoffset;
		if (yaw_alt < 0.0f)
			yaw_alt += 360.0f;
		else if (yaw_alt > 360.0f)
			yaw_alt -= 360.0f;

		pitch_alt -= yoffset;
		if (pitch_alt > 85.0f)
			pitch_alt = 85.0f;
		else if (pitch_alt < -85.0f)
			pitch_alt = -85.0f;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0) {
		fov *= 1.1;
	}
	else if (yoffset < 0) {
		fov /= 1.1;
	}

	if (fov < 1.0f)
		fov = 1.0f;
	else if (fov > 45.0f)
		fov = 45.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		mousePressed = (action == GLFW_PRESS);
		printf("Mouse pressed: %d\n", mousePressed);
	}
}


float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

void thead_test(int sleep, bool *thread_running)
{
	int i = 0;
	while (*thread_running)
	{
		printf("Hello %d\n", i++);
		std::this_thread::sleep_for(1000ms);
	}
}

int main()
{
	printf("Hello World\n");

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	if (USE_WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//Set callback functions
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glEnable(GL_DEPTH_TEST);

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffser_size_callback);

	Shader ourShader("model_loading.vs", "model_loading.fs");
	//string path_str = FileSystem::getPath("resources\\backpack\\backpack.obj"); 
	string path_str0 = FileSystem::getPath("enclosure_body.stl");
	string path_str1 = FileSystem::getPath("enclosure_base.stl");
	string path_str2 = FileSystem::getPath("enclosuer_top.stl");
	cout << "Cout: " << path_str2.c_str() << endl;
	printf("Path: %s\n", path_str2.c_str());


	Model enclosure_models[3];
	enclosure_models[0].loadModel((char*)path_str0.c_str());
	enclosure_models[1].loadModel((char*)path_str1.c_str());
	enclosure_models[2].loadModel((char*)path_str2.c_str());
	//Model ourModel((char*)path_str0.c_str());
	cout << "Models loaded" << endl;
	
	//Light cube
	//glm::vec3 lightPos(20.0f, 15.0f, 2.0f);
	
	Shader lightCubeShader("light_cube.vs", "light_cube.fs");
	Shader ledShader("light_cube.vs", "led_shader.fs");
	unsigned int VBO, lightCubeVAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glm::vec3 frame_buffer[96][8][6] = { glm::vec3(0.0f) };

	for (int i = 0; i < 96; i++) 
	{
		if (i % 6 == 0) 
		{
			for (int k = 0; k < 6; k++)
			{
				frame_buffer[i][0][k] = glm::vec3(k / 6.0f, 0.0f, (5.0 - k) / 6.0f);
			}
		}
		frame_buffer[i][i%8][3] = glm::vec3(0.0f, 1.0f, 0.2f);
	}

	//Setup framebuffer
	//doubleBuffer arduino_buffer;
	//arduino_buffer.clear();


	//Start thread
	bool thread_running = true;
	thread th1(thead_test, 1000, &thread_running);


	int cnt = 0;
	int idx = 0;
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		if (cnt % 6 == 0)
		{
			//Clear frame buffer
			for (int i = 0; i < 96; i++) {
				for (int j = 0; j < 8; j++) {
					for (int k = 0; k < 6; k++) {
						frame_buffer[i][j][k] = glm::vec3(0.0f);
					}
				}
			}
			//arduino_buffer.clear();

			//Update framebuffer
			for (int i = 0; i < 96; i++)
			{
				if (i % 6 == 0)
				{
					for (int k = 0; k < 6; k++)
					{
						frame_buffer[i][0][k] = glm::vec3(k / 6.0f, 0.0f, (5.0 - k) / 6.0f);
						//arduino_buffer.setColors(i, 0, k, (255/6)*k, 0, (255/6) * (5 - k));
					}
				}
				frame_buffer[i][(i + idx) % 8][3] = glm::vec3(0.0f, 1.0f, 0.2f);
				//arduino_buffer.setColors(i, (i + idx) % 8, 3, 0, 255, 0);
			}
			idx++;
		}
		cnt++;
		//arduino_buffer.update();

		//Rendering commands here
		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		glm::vec3 box_pos = glm::vec3(1.0f, 0.0f, 0.0);
		glm::vec4 box_vec = glm::vec4(box_pos, 1.0f);
		glm::mat4 box_transform = glm::mat4(1.0f);
		box_transform = glm::rotate(box_transform, glm::radians(yaw_alt), glm::vec3(0.0f, 1.0f, 0.0f));
		box_vec = box_transform * box_vec;
		//printf("x: %f, y: %f, z: %f\n", box_vec.x, box_vec.y, box_vec.z);
		box_pos = glm::vec3(box_vec.x, box_vec.y, box_vec.z);
		glm::vec3 axis = glm::cross(box_pos, glm::vec3(0.0f, 1.0f, 0.0f));
		//printf("ax: %f, ay: %f, az: %f\n", axis.x, axis.y, axis.z);

		box_transform = glm::mat4(1.0f);
		box_transform = glm::rotate(box_transform, glm::radians(pitch_alt), axis);
		box_vec = box_transform * box_vec;
		//printf("x: %f, y: %f, z: %f\n", box_vec.x, box_vec.y, box_vec.z);
		box_pos = cam_radius * glm::vec3(box_vec.x, box_vec.y, box_vec.z);
		static bool first = true;
		if (first) {
			printf("box_pos = (%f, %f, %f)\n", box_pos.x, box_pos.y, box_pos.z);
			first = false;
		}
		

		//Draw Models
		ourShader.use();

		ourShader.setVec3("light.position", lightPos);
		//ourShader.setVec3("viewPos", cameraPos);
		ourShader.setVec3("viewPos", box_pos);

		// light properties
		ourShader.setVec3("light.ambient", 1.0f, 1.0f, 1.0f); // note that all light colors are set at full intensity
		ourShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
		ourShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

		// material properties
		ourShader.setVec3("material.ambient", 0.0f, 0.1f, 0.06f);
		ourShader.setVec3("material.diffuse", 0.0f, 0.50980392f, 0.50980392f);
		ourShader.setVec3("material.specular", 0.50196078f, 0.50196078f, 0.50196078f);
		ourShader.setFloat("material.shininess", 32.0f);

		glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 300.0f);
		//glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 view = glm::lookAt(box_pos, glm::vec3(0.0f, 20.0f, 0.0f), cameraUp);


		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);
		float time = glfwGetTime();
		//model = glm::rotate(model, glm::radians(5.0f * glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
		//model = glm::rotate(model, (float)(0.5f * time), glm::vec3(0.0f, 1.0f, 0.0f));
		//model = glm::translate(model, glm::vec3(0.0f, 4.0f, 0.0f));
		//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		ourShader.setMat4("model", model);
		for (int i = 0; i < 3; i++)
		{
			if (i != 2) {
				enclosure_models[i].Draw(ourShader);
			}
			else {
				/*
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, 7.45f, 0.0f));
				model = glm::rotate(model, (float)(0.5f * time) + glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));


				//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
				ourShader.setMat4("model", model);
				enclosure_models[i].Draw(ourShader);
				*/
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, 74.2f, 0.0f));
				//model = glm::rotate(model, (float)(0.5f * time) + glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, glm::radians(22.5f), glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));


				//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
				ourShader.setMat4("model", model);
				enclosure_models[i].Draw(ourShader);
			}
		}

		//Draw Light cube
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		//model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lightCubeShader.setMat4("model", model);

		glBindVertexArray(lightCubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		model = glm::mat4(1.0f);
		model = glm::translate(model, box_pos);
		//model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lightCubeShader.setMat4("model", model);
		//glDrawArrays(GL_TRIANGLES, 0, 36);

		ledShader.use();
		ledShader.setMat4("projection", projection);
		ledShader.setMat4("view", view);
		//ledShader.setVec3("ledColor", glm::vec3(0.5f, 0.0f, 1.0f));
		for (int k = 0; k < 6; k++) {
			for (int i = 0; i < 96; i++) {
				for (int j = 0; j < 8; j++) {
					
					if (glm::length(frame_buffer[i][j][k]) < 0.01)
					{
						continue;
					}
					
					/*
					frameBuffer* rBuf = arduino_buffer.getReadBuffer();
					int sum = 0;
					glm::vec3 ledColor;
					for (int idx = 0; idx < 3; idx++) {
						sum += rBuf->fbuf_[i][j][k][idx];
						switch (idx) {
							case 0:
								ledColor.x = rBuf->fbuf_[i][j][k][idx] / 255.0f;
								break;
							case 1:
								ledColor.y = rBuf->fbuf_[i][j][k][idx] / 255.0f;
								break;
							case 2:
								ledColor.z = rBuf->fbuf_[i][j][k][idx] / 255.0f;
								break;
						}
					}
					if (sum == 0)
						continue;
					*/
					ledShader.setVec3("ledColor", frame_buffer[i][j][k]);
					//ledShader.setVec3("ledColor", ledColor);
					model = glm::mat4(1.0f);
					model = glm::rotate(model, glm::radians(3.75f * i), glm::vec3(0.0f, 1.0f, 0.0f));
					model = glm::translate(model, glm::vec3(35.0f + j * 5.0f, 20.1f + 7.6*k, 0.0f));
					model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
					ledShader.setMat4("model", model);
					glDrawArrays(GL_TRIANGLES, 0, 36);
				}
			}
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	thread_running = false;
	th1.join();
	return 0;
}