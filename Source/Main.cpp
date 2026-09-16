#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <system_error>
#include <vector>
#include "WindowSetting.h"
#include "Screens/StartScreen.h"
#include "Screens/GameScreen.h"

using namespace std;
using namespace glm;

namespace fs = std::filesystem;

//dimension of screen
const static int width = 1200;
const static int height = 700;

WindowSetting window_setting;
bool start_game = false;

void resize_window(GLFWwindow *window, int width, int height);
void process_inputs(GLFWwindow *window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void focus_callback(GLFWwindow* window, int focused);
void on_window_focused(GLFWwindow* window);
void unfocus_window(GLFWwindow* window);
bool resolve_resource_root(const char* argv0);
void warn_if_lfs_placeholders();

int run_game();

int main(int argc, char** argv) {
	//everything under Resources/ is loaded through paths relative to the
	//working directory, so point it at whichever directory holds Resources/
	if (!resolve_resource_root(argc > 0 ? argv[0] : nullptr)) {
		return -1;
	}
	warn_if_lfs_placeholders();

	//shader/audio loading code throws bare int/const char*/std::exception on
	//failure (missing file, no audio device, ...) with nothing upstream to
	//catch it, which otherwise crashes with an opaque OS fault dialog and no
	//indication of what went wrong.
	try {
		return run_game();
	}
	catch (const std::exception& e) {
		cerr << "Fatal error: " << e.what() << endl;
	}
	catch (const char* msg) {
		cerr << "Fatal error: " << msg << endl;
	}
	catch (int err) {
		cerr << "Fatal error: errno " << err << " (" << strerror(err) << ")" << endl;
	}
	catch (...) {
		cerr << "Fatal error: unknown exception" << endl;
	}
	cerr << "Press Enter to exit..." << endl;
	cin.get();
	return -1;
}

int run_game() {
	//initializes GLFW libraries
	glfwInit();

	//specifies OpenGL version (3.3)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//gets access to modern features, not outdated ones.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	
	//creates window
	GLFWwindow* window = glfwCreateWindow(width, height, "Voxel World", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create a window" << endl;
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwFocusWindow(window);
	glfwSetWindowFocusCallback(window, focus_callback);
	glfwSetFramebufferSizeCallback(window, resize_window);
	glfwSetScrollCallback(window, scroll_callback);
	
	gladLoadGL();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	window_setting = { window, width, height, true };
	StartScreen start_screen = StartScreen();
	GameScreen game_screen = GameScreen(&window_setting);

	//displays a screen every frame while the window is active
	while (!glfwWindowShouldClose(window)) {
		process_inputs(window);

		if (start_game) {
			game_screen.gl_settings();
			game_screen.draw();
		}
		else {
			start_screen.draw();
			StartScreen::Action action = start_screen.poll_buttons(&window_setting);
			if (action == StartScreen::Action::Start) {
				start_game = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else if (action == StartScreen::Action::Exit) {
				glfwSetWindowShouldClose(window, true);
			}
		}
		
		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying everything including window
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

/*
* callback function for window resize
*/
void resize_window(GLFWwindow *window, int width, int height) {
	//tells glad how to display data and coordinates to the window
	glViewport(0, 0, width, height);
	window_setting.width = width;
	window_setting.height = height;
}

/*
* receives user inputs and executes behaviors
*/
void process_inputs(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		unfocus_window(window);
	}
	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
		start_game = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	if (GLFW_HOVERED && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		on_window_focused(window);
	}
}

/*
* callback function for mouse scroll, used to cycle the hotbar selection
*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	window_setting.scroll_delta_y += yoffset;
}

/*
* callback function for window focus
*/
void focus_callback(GLFWwindow* window, int focused) {
	if (focused) {
		on_window_focused(window);
		window_refocused = true;
	}
	else {
		unfocus_window(window);
		window_refocused = false;
	}
}

/*
* a sub-fuction for focus_callback(). disables mouse cursor on window focused,
* but only once the game has actually started - the start menu needs a visible,
* free-moving cursor so its buttons are clickable
*/
void on_window_focused(GLFWwindow* window) {
	window_setting.window_active = true;
	glfwSetInputMode(window, GLFW_CURSOR, start_game ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

/*
* a sub-function for focus_callback(). enables mouse cursor on window unfocused
*/
void unfocus_window(GLFWwindow* window) {
	window_setting.window_active = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

/*
* sets the working directory to the directory that contains Resources/.
* candidates, in order: the executable's directory and its parents (so the game
* runs from build/Debug, build/Release, an install folder, ...), the current
* working directory, and the source directory baked in at configure time.
* returns false if Resources/ could not be found anywhere.
*/
bool resolve_resource_root(const char* argv0) {
	error_code ec;
	vector<fs::path> candidates;

	if (argv0 != nullptr && *argv0 != '\0') {
		fs::path exe = fs::weakly_canonical(fs::path(argv0), ec);
		if (!ec) {
			fs::path dir = exe.parent_path();
			//the executable typically sits 1-3 levels below the project root
			for (int i = 0; i < 5 && !dir.empty(); ++i) {
				candidates.push_back(dir);
				if (dir == dir.parent_path()) break;
				dir = dir.parent_path();
			}
		}
	}

	fs::path cwd = fs::current_path(ec);
	if (!ec) candidates.push_back(cwd);

#ifdef VOXELWORLD_SOURCE_DIR
	candidates.push_back(fs::path(VOXELWORLD_SOURCE_DIR));
#endif

	for (const fs::path& dir : candidates) {
		if (fs::exists(dir / "Resources" / "Shaders", ec)) {
			fs::current_path(dir, ec);
			if (ec) {
				cerr << "Failed to enter " << dir.string() << ": " << ec.message() << endl;
				return false;
			}
			return true;
		}
	}

	cerr << "Could not find the Resources folder.\n"
		<< "Run the game from the repository root, or keep Resources/ next to the executable." << endl;
	return false;
}

/*
* the .wav files are stored in Git LFS. Cloning without git-lfs leaves small
* text pointers in their place, which makes audio loading throw at startup with
* no useful message, so check one of them up front.
*/
void warn_if_lfs_placeholders() {
	const char* probe = "Resources/Musics/sweden.wav";
	ifstream in(probe, ios::binary);
	if (!in) return;

	char header[8] = {};
	in.read(header, sizeof(header));
	if (in.gcount() == sizeof(header) && string(header, 7) == "version") {
		cerr << "WARNING: audio files are Git LFS pointers, not real audio.\n"
			<< "Run 'git lfs install' then 'git lfs pull' in the repository, "
			<< "otherwise the game will crash when it loads sound." << endl;
	}
}