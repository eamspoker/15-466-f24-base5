#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void addTextures();

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;

		//items
		std::vector<bool> inventory = {false, false, false}; 
	} player;


	// textures
		struct PosTexVertex {
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	struct TexStruct{
		GLuint tex = 0;
		GLuint tristrip_buffer = 0; 
		GLuint tristrip_buffer_for_texture_program_vao  = 0; // vao, describes how to attach tristrip

		GLsizei count = 0; // how many things are there
		glm::mat4 CLIP_FROM_LOCAL = glm::mat4(1.0f);

		// size relative to the window
		float relativeSizeX = 0.0f;
		float relativeSizeY = 0.0f;
	} tex_example, inventory1, inventory1f, inventory2, inventory2f, inventory3, inventory3f;

	std::vector<TexStruct *> textures = {&tex_example, &inventory1, &inventory1f, &inventory2, &inventory2f, &inventory3, &inventory3f};
	std::vector<std::string> paths = {"potion1.png", "inventory1.png", "book.png", "inventory1.png", "coffeebeans.png", "inventory1.png", "spice.png" };

	// Animation
	float animTime = 0.0f;
	struct AnimObj
	{
		Scene::Transform *transform;
		glm::vec3 original_position;
	} bubbles;

	struct Door 
	{
		Scene::Transform *transform;
		glm::vec3 original_position;
		bool canOpen;
		float transition_time;
		float currTime;
		bool isClosed;
	} libraryDoor;

	std::vector<Door> doors = {libraryDoor};


	struct Item 
	{
		Scene::Transform *transform;
	} book, coffee, pumpkin_spice;

	size_t selectedObject = 0;
	std::vector<std::string> selectedObjectNames = {"None", "Book", "Beans", "Spices"};




};
