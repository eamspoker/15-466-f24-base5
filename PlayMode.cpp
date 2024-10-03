#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include  "TextureProgram.hpp"
#include "SpecialColorProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>


#include <random>

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("street.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

GLuint phonebank_meshes_for_special_color_program = 0;
Load< MeshBuffer > meshes_special(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("street.pnct"));
	phonebank_meshes_for_special_color_program = ret->make_vao_for_program(special_color_program->program);
	return ret;
});


Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("street.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		drawable.pipeline2 = special_color_program_pipeline;
		drawable.pipeline2.vao = phonebank_meshes_for_special_color_program;
		drawable.pipeline2.type = mesh.type;
		drawable.pipeline2.start = mesh.start;
		drawable.pipeline2.count = mesh.count;

	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("street.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

void PlayMode::addTextures()
{
	size_t path_index = 0;

	for (TexStruct *tex_ : textures)
	{
		assert(tex_);
		auto &tex = *tex_;
		
		// from in-class example
		glGenTextures(1, &tex.tex); 
		{
			//load texture data as RGBA from a file:
			std::vector< glm::u8vec4 > data;
			glm::uvec2 size;

			load_png(data_path(paths[path_index]), &size, &data, LowerLeftOrigin);

			glBindTexture(GL_TEXTURE_2D, tex.tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);

			// texture, level, color scheme, width height, border
			// add border bc sometimes it can get weird in linear sampling, more control over interpolation
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			// maybe some aliasing, sampling lower detail than the texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glBindTexture(GL_TEXTURE_2D, 0);
		};

		glGenBuffers(1, &tex.tristrip_buffer);
		{
			glGenVertexArrays(1, &tex.tristrip_buffer_for_texture_program_vao); 
			// make buffer of pos tex vertices
			glBindVertexArray(tex.tristrip_buffer_for_texture_program_vao);
			glBindBuffer(GL_ARRAY_BUFFER, tex.tristrip_buffer);

			//size, type, normalize, stride, offset <-- recall from reading
			// these are postex vertices
			glVertexAttribPointer(texture_program->Position_vec4,
				3,
				GL_FLOAT,
				GL_FALSE,
				sizeof(PosTexVertex),
				(GLbyte*)0 + offsetof(PosTexVertex, Position)
			);

			glEnableVertexAttribArray(texture_program->Position_vec4);

			//size, type, normalize, stride, offset <-- recall from reading
			// these are postex vertices
			glVertexAttribPointer(texture_program->TexCoord_vec2,
				2,
				GL_FLOAT,
				GL_FALSE,
				sizeof(PosTexVertex),
				(GLbyte*)0 + offsetof(PosTexVertex, TexCoord)
			);

			glEnableVertexAttribArray(texture_program->TexCoord_vec2);


			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		} 
		path_index++;
	}

	

	
}

PlayMode::PlayMode() : scene(*phonebank_scene) {
	// get pointers
	for (auto &transform : scene.transforms) {
		if (transform.name == "Bubbles") 
		{
			bubbles.transform = &transform;
			bubbles.original_position = transform.position;
		}

		if (transform.name == "LibraryDoor") 
		{
			libraryDoor.transform = &transform;
			libraryDoor.original_position = transform.position;
			libraryDoor.canOpen = true;
			libraryDoor.isClosed = true;
			libraryDoor.transition_time = 2.0f;
			libraryDoor.currTime = -1.0f;
		}

		if (transform.name == "Book") 
		{
			book.transform = &transform;
		}

		if (transform.name == "Beans") 
		{
			coffee.transform = &transform;
		}

		if (transform.name == "Spices") 
		{
			pumpkin_spice.transform = &transform;
		}
	
	}

	//create a player transform:
	scene.transforms.emplace_back();
	player.transform = &scene.transforms.back();

	player.transform->position.y = -12.0f;
	player.transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 1.0f, 0.0f));;

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;

	player.camera->transform->parent = player.transform;

	//player's eyes are 1.8 units above the ground:
	player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(player.transform->position);

	// textures
	// from in-class example

	addTextures();



}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = true;
			return true;
		} 
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			if (!player.inventory[0] && selectedObject == 1)
			{
				player.inventory[0] = true;
				selectedObject = 0;
				book.transform->position.z = -100.0f;
			}
			if (!player.inventory[1] && selectedObject == 2)
			{
				player.inventory[1] = true;
				selectedObject = 0;
				coffee.transform->position.z = -100.0f;
			}
			if (!player.inventory[2] && selectedObject == 3)
			{
				player.inventory[2] = true;
				selectedObject = 0;
				pumpkin_spice.transform->position.z = -100.0f;
			}
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / (float(window_size.y)*2.0f),
				-evt.motion.yrel / (float(window_size.y)*2.0f)
			);
			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, upDir) * player.transform->rotation;

			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			return true;
		}
	}

	bool isFirst = true;
	for (TexStruct *tex_ : textures)
	{
		assert(tex_);
		auto &tex = *tex_;

		if (isFirst)
		{
			tex.relativeSizeX = window_size.x > 0.0f ? 1000.0f/window_size.x : tex.relativeSizeX;
			tex.relativeSizeY = window_size.y > 0.0f ? 200.0f/window_size.y : tex.relativeSizeY;
			isFirst = false;
		} else {
			tex.relativeSizeX = window_size.x > 0.0f ? 250.0f/window_size.x : tex.relativeSizeX;
			tex.relativeSizeY = window_size.y > 0.0f ? 250.0f/window_size.y : tex.relativeSizeY;
		}
	}



	return false;
}

void PlayMode::update(float elapsed) {
	//player walking:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 3.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain = glm::vec3(0.0f);
				break;
			}
			
			//some step remains:
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				//rotate step to follow surface:
				remain = rotation * remain;

			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				} else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}


			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player.transform->position = walkmesh->to_world_point(player.at);

		{ //update player's rotation to respect local (smooth) up-vector:
			
			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
		}

		/*
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
		*/
	}

	// do scene animations
	{
		animTime += elapsed;
		if (animTime > 10.0f)
		{
			animTime -= 10.0f;
		}
		bubbles.transform->position.z = bubbles.original_position.z + std::sin((animTime*float(M_PI))/10.0f);

		// idea from https://stackoverflow.com/questions/3232318/sphere-sphere-collision-detection-reaction
		auto sphereCollision = [&](glm::vec3 center1, float radius1, glm::vec3 center2, float radius2)
		{
			float dist_squared = pow(glm::length(center1-center2),2.0f);
			float radii_squared = pow(radius1+radius2,2.0f);


			return dist_squared < radii_squared;
		};

		// selecting objects
		if (sphereCollision(player.transform->position, 1.0f, book.transform->position, 2.0f))
		{
			selectedObject = 1;
		} else if (sphereCollision(player.transform->position, 1.0f, coffee.transform->position, 2.0f))
		{ 
			selectedObject = 2;
		} else if (sphereCollision(player.transform->position, 1.0f, pumpkin_spice.transform->position, 2.0f))
		{ 
			selectedObject = 3;
		} else if (selectedObject != 0)
		{
			selectedObject = 0;
		}

		// door animations
		if (libraryDoor.canOpen)
		{
			bool isCollide = sphereCollision(player.transform->position, 1.0f, libraryDoor.original_position, 5.0f);
			if(libraryDoor.isClosed &&
				isCollide)
			{

				if (libraryDoor.currTime < 0.0f)
				{
					libraryDoor.currTime = 0.0f;
				}

				libraryDoor.currTime += elapsed;
					
				if (libraryDoor.transition_time >= libraryDoor.currTime)
				{
						libraryDoor.transform->position.y = libraryDoor.original_position.y 
						+ 5.0f*(libraryDoor.currTime/libraryDoor.transition_time);

				} else {
					libraryDoor.currTime = -1.0f;
					libraryDoor.isClosed = false;

				}
			
			} else if (!libraryDoor.isClosed && 
						!isCollide)
			{
				if (libraryDoor.currTime < 0.0f)
				{
					libraryDoor.currTime = libraryDoor.transition_time;
				}

				libraryDoor.currTime -= elapsed;
				
				if (libraryDoor.currTime >= 0.0f)
				{
						libraryDoor.transform->position.y = libraryDoor.original_position.y 
						+ 5.0f*(libraryDoor.currTime/libraryDoor.transition_time);

				} else {
					libraryDoor.currTime = -1.0f;
					libraryDoor.isClosed = true;
				}
			} else if (libraryDoor.isClosed && 
						libraryDoor.currTime > 0.0f &&
						!isCollide)
			{

				libraryDoor.currTime -= elapsed;

				
				if (libraryDoor.currTime >= 0.0f)
				{
						libraryDoor.transform->position.y = libraryDoor.original_position.y 
						+ 5.0f*(libraryDoor.currTime/libraryDoor.transition_time);

				} else {
					std::cout << "here2" << std::endl;

					libraryDoor.currTime = -1.0f;
				}
			}
		}
		

	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// texture update tristrip
	{
		auto texUpdate = [&] (TexStruct *tex, bool rightAligned, float offset){
		// don't do this if back face culling
		std::vector<PosTexVertex> verts;

		// pin to right corner
		if (rightAligned)
		{
			verts.emplace_back(PosTexVertex{
			.Position = glm::vec3(1.0f-tex->relativeSizeX, 1.0f-tex->relativeSizeY, 0.0f),
			.TexCoord = glm::vec2(0.0f, 0.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(1.0f-tex->relativeSizeX, 1.0f, 0.0f),
				.TexCoord = glm::vec2(0.0f, 1.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(1.0f, 1.0f-tex->relativeSizeY, 0.0f),
				.TexCoord = glm::vec2(1.0f, 0.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(1.0f, 1.0f, 0.0f),
				.TexCoord = glm::vec2(1.0f, 1.0f),
			});

		} else {
			// pin to left corner (for inventory items)
			verts.emplace_back(PosTexVertex{
			.Position = glm::vec3(-1.0f+(offset*tex->relativeSizeX), 1.0f - tex->relativeSizeY, 0.0f),
			.TexCoord = glm::vec2(0.0f, 0.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(-1.0f+(offset*tex->relativeSizeX), 1.0f, 0.0f),
				.TexCoord = glm::vec2(0.0f, 1.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(-1.0f+tex->relativeSizeX+(offset*tex->relativeSizeX), 1.0f - tex->relativeSizeY, 0.0f),
				.TexCoord = glm::vec2(1.0f, 0.0f),
			});

			verts.emplace_back(PosTexVertex{
				.Position = glm::vec3(-1.0f+tex->relativeSizeX+(offset*tex->relativeSizeX), 1.0f, 0.0f),
				.TexCoord = glm::vec2(1.0f, 1.0f),
			});

		}


	glBindBuffer(GL_ARRAY_BUFFER, tex->tristrip_buffer);

	// tells us update:  GL_STREAM_DRAW, stream = update once a frame, draw = what we're gonna do
	// could read, draw copy etc.
	// worst case, run slower, telling about gpu memory, fast for static or stream from memory
	glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(verts[0]), verts.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	tex->count = verts.size();
	};

	texUpdate(&tex_example, true, 0);

	if (!player.inventory[0])
	{
		texUpdate(&inventory1, false, 0);
	} else {
		texUpdate(&inventory1f, false, 0);
	}

	if (!player.inventory[1])
	{
		texUpdate(&inventory2, false, 1);
	} else {
		texUpdate(&inventory2f, false, 1);
	}

	if (!player.inventory[2])
	{
		texUpdate(&inventory3, false, 2);
	} else {
		texUpdate(&inventory3f, false, 2);
	}
	
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera, selectedObjectNames[selectedObject]);

	/* In case you are wondering if your walkmesh is lining up with your scene, try:
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : walkmesh->triangles) {
			lines.draw(walkmesh->vertices[tri.x], walkmesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.y], walkmesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.z], walkmesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
		}
	}
	*/

{
	// from in-class example

		// TODO: add alpha

		
		auto drawTex = [&](TexStruct *tex) {

			glUseProgram(texture_program->program);
			glBindVertexArray(tex->tristrip_buffer_for_texture_program_vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex->tex);
			// number, transposed or not
			glUniformMatrix4fv(texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, glm::value_ptr(tex->CLIP_FROM_LOCAL));
			glDrawArrays(GL_TRIANGLE_STRIP, 0, tex->count);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
			glUseProgram(0);
		};

		drawTex(&tex_example);
		if (!player.inventory[0])
		{
			drawTex(&inventory1);
		} else {
			drawTex(&inventory1f);
		}
		
		if (!player.inventory[1])
		{
			drawTex(&inventory2);
		} else {
			drawTex(&inventory2f);
		}
		
		if (!player.inventory[2])
		{
			drawTex(&inventory3);
		} else {
			drawTex(&inventory3f);
		}
		
}

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse; space to pick up objects",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse; space to pick up objects",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
