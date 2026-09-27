/*
	measures the real chunk pipeline and terrain rendering in a hidden window:
	loading the render area, per-chunk mesh cost, mesh memory, per-frame draw
	cost from the surface and from inside a cave, and streaming while walking.
	run it from the repository root so Resources/ resolves.
*/
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static void print_stack(CONTEXT context) {
	HANDLE process = GetCurrentProcess();
	STACKFRAME64 frame = {};
	frame.AddrPC.Offset = context.Rip; frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rbp; frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp; frame.AddrStack.Mode = AddrModeFlat;
	for (int i = 0; i < 24 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context,
		nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr); ++i) {
		char buffer[sizeof(SYMBOL_INFO) + 256] = {};
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 255;
		DWORD64 offset = 0;
		IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
		DWORD line_offset = 0;
		bool named = SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol);
		bool lined = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_offset, &line);
		printf("  %s", named ? symbol->Name : "?");
		if (lined) printf("  %s:%lu", line.FileName, line.LineNumber);
		printf("\n");
	}
}

//prints where a crash happened, since the benchmark usually runs without a debugger attached
static LONG WINAPI report_crash(EXCEPTION_POINTERS* info) {
	EXCEPTION_RECORD* record = info->ExceptionRecord;
	DWORD64 at = (DWORD64)record->ExceptionAddress;
	printf("\ncrash 0x%08lx at 0x%llx, %s address 0x%llx, thread %lu\n", record->ExceptionCode, at,
		record->ExceptionInformation[0] ? "writing" : "reading", (unsigned long long)record->ExceptionInformation[1], GetCurrentThreadId());
	char buffer[sizeof(SYMBOL_INFO) + 256] = {};
	SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = 255;
	DWORD64 offset = 0;
	IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
	DWORD line_offset = 0;
	HMODULE module = nullptr;
	char module_name[MAX_PATH] = "?";
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)at, &module))
		GetModuleFileNameA(module, module_name, MAX_PATH);
	printf("  module %s +0x%llx\n", module_name, at - (DWORD64)module);
	if (SymFromAddr(GetCurrentProcess(), at, &offset, symbol)) printf("  in %s", symbol->Name);
	if (SymGetLineFromAddr64(GetCurrentProcess(), at, &line_offset, &line)) printf("  %s:%lu", line.FileName, line.LineNumber);
	printf("\n");
	print_stack(*info->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef _DEBUG
#include <crtdbg.h>
//a debug build checks every container access; report the first bad one with its stack instead of a dialog
static int report_debug_failure(int, char* message, int*) {
	printf("\n%s", message);
	CONTEXT context;
	RtlCaptureContext(&context);
	print_stack(context);
	fflush(stdout);
	ExitProcess(3);
}
#endif
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cstdio>
#include <vector>
#include "World/Terrain.h"
#include "Screens/PlayerRenderer.h"
#include "Screens/MobRenderer.h"
#include "World/MobManager.h"
#include "World/Commands.h"
#include "World/Onsen.h"
#include <map>
#include <functional>


using namespace std;
using namespace glm;

namespace {
	long long draw_calls = 0, drawn_indices = 0;
	PFNGLDRAWELEMENTSPROC real_draw_elements = nullptr;

	void APIENTRY counting_draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
		draw_calls++;
		drawn_indices += count;
		real_draw_elements(mode, count, type, indices);
	}

	PFNGLDRAWARRAYSPROC real_draw_arrays = nullptr;
	void APIENTRY counting_draw_arrays(GLenum mode, GLint first, GLsizei count) {
		draw_calls++;
		real_draw_arrays(mode, first, count);
	}

	double now_ms() {
		return chrono::duration<double, milli>(chrono::steady_clock::now().time_since_epoch()).count();
	}

	void set_view_uniforms(ShaderManager& sm, const mat4& view_projection, vec3 eye) {
		const vec3 sun = normalize(vec3(-0.4f, -0.8f, -0.3f));
		Shader* shaders[4] = { &sm.default_shader, &sm.wave_shader, &sm.foliage_shader, &sm.mob_shader };
		for (Shader* s : shaders) {
			s->activate();
			s->set_uniform_mat4f("cam_matrix", 1, GL_FALSE, view_projection);
			s->set_uniform_3f("cam_pos", 1, eye);
			s->set_uniform_3f("fog_color", 1, vec3(0.6f, 0.75f, 0.95f));
			s->set_uniform_1f("fog_start", 90.0f);
			s->set_uniform_1f("fog_end", 160.0f);
			s->set_uniform_3f("sun_direction", 1, sun);
			s->set_uniform_3f("light_color", 1, vec3(1.0f));
			s->set_uniform_1f("ambient_strength", 0.45f);
			s->set_uniform_1f("diffuse_strength", 0.55f);
			s->set_uniform_1f("time", 0.0f);
		}
	}

	struct FrameResult {
		double cpu_ms = 0, gpu_ms = 0, visibility_ms = 0;
		double calls = 0, triangles = 0, sections = 0, sections_total = 0;
	};

	FrameResult measure_frames(Terrain& terrain, ShaderManager& sm, vec3 eye, GLuint query, int frames) {
		FrameResult r;
		const mat4 projection = perspective(radians(70.0f), 1200.0f / 700.0f, 0.1f, 180.0f);
		int samples = 0;
		for (int yaw_step = 0; yaw_step < 4; ++yaw_step) {
			float yaw = yaw_step * 1.5707963f;
			vec3 forward = normalize(vec3(cos(yaw), -0.15f, sin(yaw)));
			mat4 view_projection = projection * lookAt(eye, eye + forward, vec3(0, 1, 0));
			for (int f = 0; f < frames; ++f) {
				terrain.update_chunks();
				set_view_uniforms(sm, view_projection, eye);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw_calls = drawn_indices = 0;

				glBeginQuery(GL_TIME_ELAPSED, query);
				double t0 = now_ms();
				terrain.draw(view_projection);
				double cpu = now_ms() - t0;
				glEndQuery(GL_TIME_ELAPSED);
				GLuint64 ns = 0;
				glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ns);

				if (f == 0) continue;
				r.cpu_ms += cpu;
				r.gpu_ms += ns / 1e6;
				r.visibility_ms += terrain.stats.visibility_ms;
				r.calls += draw_calls;
				r.triangles += drawn_indices / 3.0;
				r.sections += terrain.stats.sections_drawn;
				r.sections_total += terrain.stats.sections_total;
				samples++;
			}
		}
		double n = std::max(samples, 1);
		r.cpu_ms /= n; r.gpu_ms /= n; r.visibility_ms /= n;
		r.calls /= n; r.triangles /= n; r.sections /= n; r.sections_total /= n;
		return r;
	}

	GLuint make_target(int width, int height) {
		GLuint target, color, depth;
		glGenFramebuffers(1, &target);
		glBindFramebuffer(GL_FRAMEBUFFER, target);
		glGenRenderbuffers(1, &color);
		glBindRenderbuffer(GL_RENDERBUFFER, color);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
		return target;
	}

	//renders one fixed view into the bound 1200x700 target and saves it, so two builds can be compared pixel for pixel
	void save_view(Terrain& terrain, ShaderManager& sm, Texture& atlas, vec3 eye, float yaw, const char* path) {
		const mat4 projection = perspective(radians(70.0f), 1200.0f / 700.0f, 0.1f, 180.0f);
		vec3 forward = normalize(vec3(cos(yaw), -0.15f, sin(yaw)));
		mat4 view_projection = projection * lookAt(eye, eye + forward, vec3(0, 1, 0));
		terrain.update_chunks();
		set_view_uniforms(sm, view_projection, eye);
		atlas.activate();
		atlas.bind();
		glClearColor(0.6f, 0.75f, 0.95f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		terrain.draw(view_projection);
		vector<unsigned char> pixels(1200 * 700 * 3);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, 1200, 700, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
		ofstream file(path, ios::binary);
		file << "P6\n1200 700\n255\n";
		for (int y = 699; y >= 0; --y) file.write(reinterpret_cast<const char*>(&pixels[(size_t)y * 1200 * 3]), 1200 * 3);
	}

	void print_frame(const char* label, const FrameResult& r) {
		printf("  %-26s draw %.2f ms cpu, %.2f ms gpu | %.0f draw calls, %.0fk triangles | sections %.0f/%.0f, visibility %.3f ms\n",
			label, r.cpu_ms, r.gpu_ms, r.calls, r.triangles / 1000.0, r.sections, r.sections_total, r.visibility_ms);
	}
}

int main(int argc, char** argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	int render_dist = -1;
	bool heap_check = false;
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if (arg.rfind("render_dist=", 0) == 0) render_dist = atoi(arg.c_str() + 12);
		if (arg == "heapcheck") heap_check = true;
	}
#ifdef _WIN32
	//symbols are loaded up front: after a crash the heap may be too damaged to allocate
	SymInitialize(GetCurrentProcess(), nullptr, TRUE);
	SetUnhandledExceptionFilter(report_crash);
#ifdef _DEBUG
	_CrtSetReportHook(report_debug_failure);
#endif
#endif
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(64, 64, "render_bench", nullptr, nullptr);
	if (!window) { printf("could not create a GL context\n"); return 1; }
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { printf("could not load GL\n"); return 1; }
	printf("GPU: %s\n", (const char*)glGetString(GL_RENDERER));
#ifdef _WIN32
	printf("main thread %lu\n", GetCurrentThreadId());
#endif

	real_draw_elements = glad_glDrawElements;
	glad_glDrawElements = counting_draw_elements;
	real_draw_arrays = glad_glDrawArrays;
	glad_glDrawArrays = counting_draw_arrays;

	//the game draws at window size; an offscreen target of the same size stands in for it
	GLuint fbo, color, depth, query;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenRenderbuffers(1, &color);
	glBindRenderbuffer(GL_RENDERBUFFER, color);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1200, 700);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1200, 700);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
	glViewport(0, 0, 1200, 700);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glGenQueries(1, &query);

	ShaderManager& sm = ShaderManager::get_instance();
	ChunkManager& cm = ChunkManager::get_instance();

	vec3 position = vec3(8.5f, (float)get_terrain_generator().sample_height(8, 8) + 2.0f, 8.5f);
	Terrain terrain(position);
	if (render_dist > 0) {
		terrain.render_dist = render_dist;
		terrain.keep_dist = render_dist + 3;
	}

	printf("\nloading the render area (%dx%d chunks)\n", terrain.render_dist * 2 + 1, terrain.render_dist * 2 + 1);
	terrain.build_budget_ms = 1e9f;
	double t0 = now_ms();
	terrain.update_chunks();
	double first = now_ms() - t0;
	printf("  generate %d chunks: %.1f ms (parallel), build %d chunks: %.1f ms, whole frame %.1f ms\n",
		terrain.stats.chunks_generated, terrain.stats.generate_ms, terrain.stats.chunks_built, terrain.stats.chunk_build_ms, first);
	terrain.draw(mat4(1.0f));

	vector<Chunk*> loaded;
	for (auto& e : cm.chunks) if (e.second.has_built) loaded.push_back(&e.second);

	Chunk::MeshStats total;
	for (Chunk* c : loaded) {
		Chunk::MeshStats s = c->mesh_stats();
		total.vertices += s.vertices; total.indices += s.indices;
		total.gpu_bytes += s.gpu_bytes; total.cpu_bytes += s.cpu_bytes;
	}
	double n = (double)loaded.size();
	printf("\nmesh (%zu chunks)\n", loaded.size());
	printf("  per chunk: %.0f vertices, %.0f triangles, %.1f KB on the GPU, %.1f KB in RAM\n",
		total.vertices / n, total.indices / 3.0 / n, total.gpu_bytes / 1024.0 / n, total.cpu_bytes / 1024.0 / n);
	printf("  total: %.1f MB GPU, %.1f MB RAM\n", total.gpu_bytes / 1048576.0, total.cpu_bytes / 1048576.0);

	{
		//best of several passes, since other work on the machine only ever adds time
		const int N = std::min<int>(80, (int)loaded.size());
		double best_mesh = 1e9, best_upload = 1e9;
		for (int pass = 0; pass < 5; ++pass) {
			double mesh_ms = 0, upload_ms = 0;
			for (int i = 0; i < N; ++i) {
				double a = now_ms();
				loaded[i]->build_mesh();
				double b = now_ms();
				loaded[i]->upload_mesh();
				glFinish();
				mesh_ms += b - a;
				upload_ms += now_ms() - b;
			}
			best_mesh = std::min(best_mesh, mesh_ms / N);
			best_upload = std::min(best_upload, upload_ms / N);
		}
		printf("  build_mesh %.3f ms per chunk (single thread, best of 5), upload %.3f ms per chunk\n", best_mesh, best_upload);
	}

	printf("\nframe cost (4 directions, 1200x700, shadows excluded)\n");
	vec3 surface_eye = position + vec3(0, 1.6f, 0);
	FrameResult surface_on = measure_frames(terrain, sm, surface_eye, query, 12);
	terrain.occlusion_culling = false;
	FrameResult surface_off = measure_frames(terrain, sm, surface_eye, query, 12);
	terrain.occlusion_culling = true;
	print_frame("surface, culling on", surface_on);
	print_frame("surface, culling off", surface_off);

	vec3 cave_eye = vec3(0);
	bool found = false;
	for (int x = 0; x < 64 && !found; ++x)
		for (int z = 0; z < 64 && !found; ++z)
			for (int y = 6; y < 30 && !found; ++y) {
				Block* here = cm.get_block_worldspace(vec3(x, y, z));
				Block* above = cm.get_block_worldspace(vec3(x, y + 1, z));
				Block* below = cm.get_block_worldspace(vec3(x, y - 1, z));
				if (here && above && below && here->type == none && above->type == none && is_solid(below->type)) {
					cave_eye = vec3(x + 0.5f, y + 1.6f, z + 0.5f);
					found = true;
				}
			}
	if (found) {
		position = cave_eye - vec3(0, 1.6f, 0);
		terrain.update_chunks();
		FrameResult cave_on = measure_frames(terrain, sm, cave_eye, query, 12);
		terrain.occlusion_culling = false;
		FrameResult cave_off = measure_frames(terrain, sm, cave_eye, query, 12);
		terrain.occlusion_culling = true;
		print_frame("cave, culling on", cave_on);
		print_frame("cave, culling off", cave_off);
		position = vec3(8.5f, (float)get_terrain_generator().sample_height(8, 8) + 2.0f, 8.5f);
		terrain.update_chunks();
	}

	double shadow_cpu_ms = 0, shadow_gpu_ms = 0;
	GLuint shadow_texture = 0;
	mat4 shadow_light = mat4(1.0f);
	{
		//mirrors GameScreen's shadow pass and Camera::update_light_space_matrix
		const int resolution = 2048;
		const float extent = 60.0f;
		GLuint shadow_fbo, shadow_depth;
		glGenFramebuffers(1, &shadow_fbo);
		glGenTextures(1, &shadow_depth);
		glBindTexture(GL_TEXTURE_2D, shadow_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		vec3 sun_direction = normalize(vec3(-0.4f, -0.8f, -0.3f));
		vec3 eye = position + vec3(0, 1.6f, 0);
		mat4 light_matrix = ortho(-extent, extent, -extent, extent, 1.0f, 300.0f)
			* lookAt(eye - sun_direction * 150.0f, eye, vec3(0.0f, 1.0f, 0.0f));
		double cpu = 0, gpu = 0, calls = 0, triangles = 0;
		const int frames = 20;
		for (int f = 0; f <= frames; ++f) {
			terrain.update_chunks();
			glViewport(0, 0, resolution, resolution);
			glClear(GL_DEPTH_BUFFER_BIT);
			sm.shadow_shader.activate();
			sm.shadow_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, light_matrix);
			sm.shadow_shader.set_uniform_1f("time", 0.0f);
			draw_calls = drawn_indices = 0;
			glBeginQuery(GL_TIME_ELAPSED, query);
			double t = now_ms();
			terrain.draw_shadow_casters(light_matrix);
			double c = now_ms() - t;
			glEndQuery(GL_TIME_ELAPSED);
			GLuint64 ns = 0;
			glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ns);
			if (f == 0) continue;
			cpu += c; gpu += ns / 1e6; calls += draw_calls; triangles += drawn_indices / 3.0;
		}
		printf("\nshadow pass (%dx%d depth, redrawn every 3rd frame and every frame while chunks build)\n", resolution, resolution);
		printf("  %.2f ms cpu, %.2f ms gpu | %.0f draw calls, %.0fk triangles\n",
			cpu / frames, gpu / frames, calls / frames, triangles / frames / 1000.0);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, 1200, 700);
		shadow_cpu_ms = cpu / frames;
		shadow_gpu_ms = gpu / frames;
		shadow_texture = shadow_depth;
		shadow_light = light_matrix;
	}

	{
		//from here the main pass samples the shadow map, as it does in the game
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadow_texture);
		glActiveTexture(GL_TEXTURE0);
		Shader* shaded[3] = { &sm.default_shader, &sm.wave_shader, &sm.foliage_shader };
		for (Shader* s : shaded) {
			s->activate();
			s->set_uniform_1i("shadow_map", 4);
			s->set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_light);
		}
		vec3 eye = position + vec3(0, 1.6f, 0);

		printf("\nwhere the GPU time goes (surface camera, main pass with shadows sampled)\n");
		const int sizes[4][2] = { {160, 93}, {600, 350}, {1200, 700}, {2400, 1400} };
		for (const auto& size : sizes) {
			glBindFramebuffer(GL_FRAMEBUFFER, make_target(size[0], size[1]));
			glViewport(0, 0, size[0], size[1]);
			FrameResult r = measure_frames(terrain, sm, eye, query, 6);
			printf("  %4dx%-4d (%5.2f Mpx): %.2f ms gpu\n", size[0], size[1], size[0] * size[1] / 1e6, r.gpu_ms);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, 1200, 700);

		terrain.sort_opaque_front_to_back = false;
		FrameResult unsorted = measure_frames(terrain, sm, eye, query, 12);
		terrain.sort_opaque_front_to_back = true;
		FrameResult sorted = measure_frames(terrain, sm, eye, query, 12);
		printf("  solid chunks in grid order:     %.2f ms gpu, %.2f ms cpu\n", unsorted.gpu_ms, unsorted.cpu_ms);
		printf("  solid chunks nearest first:     %.2f ms gpu, %.2f ms cpu\n", sorted.gpu_ms, sorted.cpu_ms);

		//a settled frame: nothing to generate or build, just the per-frame bookkeeping
		double idle_ms = 0;
		for (int i = 0; i < 20; ++i) {
			double a = now_ms();
			terrain.update_chunks();
			idle_ms += now_ms() - a;
			terrain.draw(mat4(1.0f));
		}
		idle_ms /= 20;
		double cpu_frame = idle_ms + sorted.cpu_ms + shadow_cpu_ms / 3.0;
		double gpu_frame = sorted.gpu_ms + shadow_gpu_ms / 3.0;
		printf("\nper-frame split at 1200x700 (shadows averaged over their 3-frame interval)\n");
		printf("  CPU %.2f ms  = update %.2f + draw submission %.2f + shadow submission %.2f\n",
			cpu_frame, idle_ms, sorted.cpu_ms, shadow_cpu_ms / 3.0);
		printf("  GPU %.2f ms  = main pass %.2f + shadow pass %.2f\n", gpu_frame, sorted.gpu_ms, shadow_gpu_ms / 3.0);

		Texture atlas("Resources/Textures/texture_atlas_blocks.png", GL_TEXTURE1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		for (Shader* s : shaded) {
			s->activate();
			s->set_uniform_1i("texture1", 1);
		}
		save_view(terrain, sm, atlas, eye, 0.3f, "bench_view_a.ppm");
		save_view(terrain, sm, atlas, eye, 3.4f, "bench_view_b.ppm");

		//everything in GameScreen's frame that is not terrain
		WindowSetting setting = { window, 1200, 700, true, 0.0 };
		PlayerRenderer renderer(&setting);
		Inventory inventory;
		const mat4 projection = perspective(radians(70.0f), 1200.0f / 700.0f, 0.1f, 180.0f);
		const mat4 view = lookAt(eye, eye + normalize(vec3(1.0f, -0.15f, 0.3f)), vec3(0, 1, 0));
		const vec3 to_sun = -normalize(vec3(-0.4f, -0.8f, -0.3f));
		auto timed = [&](const char* label, const function<void()>& pass) {
			double cpu = 0, gpu = 0, calls = 0;
			const int frames = 10;
			for (int f = 0; f <= frames; ++f) {
				renderer.bind_fbo();
				draw_calls = 0;
				glBeginQuery(GL_TIME_ELAPSED, query);
				double t = now_ms();
				pass();
				double c = now_ms() - t;
				glEndQuery(GL_TIME_ELAPSED);
				GLuint64 ns = 0;
				glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ns);
				if (f == 0) continue;
				cpu += c; gpu += ns / 1e6; calls += draw_calls;
			}
			printf("  %-22s %.3f ms cpu, %.3f ms gpu, %.0f draw calls\n", label, cpu / frames, gpu / frames, calls / frames);
		};
		printf("\nrest of the frame (1200x700)\n");
		timed("sky background", [&] {
			glDisable(GL_DEPTH_TEST);
			renderer.draw_sky_background(view, projection, vec3(0.25f, 0.45f, 0.85f), vec3(0.6f, 0.75f, 0.95f), to_sun);
			glEnable(GL_DEPTH_TEST);
		});
		timed("sun and moon", [&] { renderer.draw_sky_discs(view, projection, eye, to_sun); });
		timed("clouds", [&] { renderer.draw_clouds(projection * view, vec2(eye.x, eye.z), 0.0f, vec3(1.0f)); });
		timed("crosshair", [&] { renderer.draw_HUDs(); });
		timed("hotbar", [&] { renderer.draw_hotbar(inventory); });
		timed("F3 overlay", [&] { renderer.draw_perf_overlay(terrain.stats); });
		{
			renderer.bind_fbo();
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderer.draw_perf_overlay(terrain.stats);
			vector<unsigned char> pixels(1200 * 700 * 3);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, 1200, 700, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
			ofstream file("bench_overlay.ppm", ios::binary);
			file << "P6\n1200 700\n255\n";
			for (int y = 699; y >= 0; --y) file.write(reinterpret_cast<const char*>(&pixels[(size_t)y * 1200 * 3]), 1200 * 3);
		}
		timed("post-process to screen", [&] {
			renderer.unbind_fbo();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			sm.frame_buffer_shader.activate();
			renderer.post_process();
			glEnable(GL_DEPTH_TEST);
		});
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, 1200, 700);
	}

	{
		const int frames = 1800;
		printf("\nmobs (%d frames simulated at 60 fps)\n", frames);
		MobManager mobs;
		MobRenderer mob_renderer;
		MobContext context;
		context.player_eye = position + vec3(0.0f, 0.72f, 0.0f);
		map<const Mob*, vec3> first_seen;
		long overlap_frames = 0, mob_frames = 0;
		PlayerPhysics probe_physics;
		//a resting mob touches its floor to within float error, so only count real penetration
		auto penetrates = [&](const Mob& m) { GameObject probe = m; probe.size -= vec3(0.02f); return !probe_physics.is_position_clear(probe); };
		auto block_at = [&](vec3 p) { return cm.get_block_worldspace(vec3(std::round(p.x), std::round(p.y), std::round(p.z))); };
		double update_ms = 0, build_ms = 0;
		for (int f = 0; f < frames; ++f) {
			double a = now_ms();
			mobs.update(1.0f / 60.0f, context);
			double b = now_ms();
			mob_renderer.build(mobs.all());
			build_ms += now_ms() - b;
			update_ms += b - a;
			for (const auto& m : mobs.all()) {
				first_seen.emplace(m.get(), m->position);
				mob_frames++;
				bool pen = penetrates(*m);
				if (pen) overlap_frames++;
			}
		}
		int standing = 0, wet = 0, embedded = 0;
		float moved_total = 0, moved_max = 0;
		map<string, int> per_type;
		for (const auto& m : mobs.all()) {
			per_type[m->type.name]++;
			vec3 feet = m->feet();
			Block* ground = block_at(feet - vec3(0, 0.5f, 0));
			float ground_top = std::round(feet.y - 0.5f) + 0.5f;
			if (ground && is_solid(ground->type) && std::abs(feet.y - ground_top) < 0.05f) standing++;
			if (penetrates(*m)) embedded++;
			Block* body = block_at(m->position);
			if (body && body->type == water) wet++;
			float moved = length(vec2(m->position.x, m->position.z) - vec2(first_seen[m.get()].x, first_seen[m.get()].z));
			moved_total += moved;
			moved_max = std::max(moved_max, moved);
		}
		int n = (int)mobs.all().size();
		printf("  %d mobs alive (%zu spawned in total):", n, first_seen.size());
		for (const auto& entry : per_type) printf(" %d %s", entry.second, entry.first.c_str());
		printf("\n  %d standing on solid ground, %d overlapping a block, %d in water\n", standing, embedded, wet);
		printf("  hitbox overlapped a block in %ld of %ld mob-frames\n", overlap_frames, mob_frames);
		printf("  distance from spawn: %.1f blocks on average, %.1f at most\n", n ? moved_total / n : 0.0f, moved_max);
		printf("  cost per frame: update %.3f ms, pose upload %.3f ms\n", update_ms / frames, build_ms / frames);

		//a close look at a mob, lit and shadowed like the game draws it
		mat4 shot_light = shadow_light; //the aquatic shots are far from the shadow map and switch it off
		vec3 custom_eye = vec3(0.0f), custom_target = vec3(0.0f); //for view 3
		//view 0 from the side, 1 a chase view from behind and above (a flier's wingspan), 2 face on and close, 3 from custom_eye at custom_target
		auto shoot = [&](const Mob& subject, const string& name, const MobManager& source, const Mob* extra = nullptr, int view = 0) {
			vec3 side = vec3(cos(subject.yaw), 0.0f, -sin(subject.yaw));
			vec3 ahead = vec3(sin(subject.yaw), 0.0f, cos(subject.yaw));
			//the first spot beside the mob, a little ahead so the face shows, with a clear line of sight to it
			float back = 3.0f + subject.size.x * 2.5f; //further out for the big ones
			vec3 eye = subject.position + side * back + vec3(0.0f, 0.6f, 0.0f);
			bool found = view != 0;
			if (view == 1) eye = subject.position - ahead * back * 1.3f + vec3(0.0f, back * 0.6f, 0.0f);
			vec3 target = subject.position;
			if (view == 2) {
				//straight at the face, level with the head
				target = subject.position + ahead * (subject.size.x * 0.6f) + vec3(0.0f, 0.2f, 0.0f);
				eye = target + ahead * 1.8f + vec3(0.0f, 0.15f, 0.0f);
			}
			if (view == 3) {
				eye = custom_eye;
				target = custom_target;
			}
			for (float lift : { 0.6f, 1.6f, 2.6f, 4.0f }) {
				for (float flip : { 1.0f, -1.0f }) {
					vec3 candidate = subject.position + side * (back * flip) + ahead * 1.5f + vec3(0.0f, lift, 0.0f);
					bool clear = true;
					for (float t = 0.15f; t <= 1.0f && clear; t += 0.05f) {
						Block* b = cm.get_block_worldspace(round(mix(subject.position, candidate, t)));
						clear = b == nullptr || b->type == none || b->type == water;
					}
					if (clear && !found) { eye = candidate; found = true; }
				}
			}
			mat4 view_projection = perspective(radians(60.0f), 1200.0f / 700.0f, 0.1f, 180.0f)
				* lookAt(eye, target, vec3(0, 1, 0));
			Texture atlas("Resources/Textures/texture_atlas_blocks.png", GL_TEXTURE1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
			for (Shader* s : { &sm.default_shader, &sm.wave_shader, &sm.foliage_shader, &sm.mob_shader }) {
				s->activate();
				s->set_uniform_1i("texture1", s == &sm.mob_shader ? MobRenderer::skin_unit : 1);
				s->set_uniform_1i("shadow_map", 4);
				s->set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shot_light);
			}
			sm.mob_shader.set_uniform_1i("part_matrices", MobRenderer::matrix_unit);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, shadow_texture);
			//visibility is traced from the player, not this camera
			terrain.occlusion_culling = false;
			terrain.update_chunks();
			terrain.occlusion_culling = true;
			set_view_uniforms(sm, view_projection, eye);
			atlas.activate();
			atlas.bind();
			glClearColor(0.6f, 0.75f, 0.95f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			terrain.draw_opaque(view_projection);
			mob_renderer.build(source.all(), extra);
			mob_renderer.draw();
			terrain.draw_translucent();
			vector<unsigned char> pixels(1200 * 700 * 3);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, 1200, 700, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
			ofstream file("bench_mob_" + name + ".ppm", ios::binary);
			file << "P6\n1200 700\n255\n";
			for (int y = 699; y >= 0; --y) file.write(reinterpret_cast<const char*>(&pixels[(size_t)y * 1200 * 3]), 1200 * 3);
		};

		//the nearest mob of each type, inside the player's shadow map and loaded chunks
		map<string, Mob*> subjects;
		for (const auto& m : mobs.all()) {
			Mob*& best = subjects[m->type.name];
			if (best == nullptr || distance(m->position, position) < distance(best->position, position)) best = m.get();
		}
		for (const auto& entry : subjects) shoot(*entry.second, entry.first, mobs);

		printf("\ncombat\n");
		auto step = [&](int count) { for (int i = 0; i < count; ++i) mobs.update(1.0f / 60.0f, context); };
		auto alive = [&](const Mob* m) {
			for (const auto& x : mobs.all()) if (x.get() == m) return true;
			return false;
		};
		auto flat = [](vec3 v) { return vec3(v.x, 0.0f, v.z); };
		if (!subjects.empty()) {
			Mob* victim = subjects.begin()->second;
			float d = 0.0f;
			Mob* picked = mobs.pick(victim->position + vec3(3.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), 4.5f, d);
			float miss_d = 0.0f;
			Mob* missed = mobs.pick(victim->position + vec3(3.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), 4.5f, miss_d);
			printf("  pick along a ray from 3 blocks away: %s at %.2f (hitbox side at %.2f); pointing away: %s\n",
				picked == victim ? "hit" : "wrong mob", d, 3.0f - victim->size.x * 0.5f, missed == victim ? "hit (wrong)" : "no hit");

			vec3 source = victim->position - vec3(2.0f, 0.0f, 0.0f);
			vec3 start = victim->position;
			float health_before = victim->health;
			bool first = victim->hurt(2.0f, source);
			bool second = victim->hurt(2.0f, source);
			step(30);
			printf("  %s %s: first hit %s, a second hit at once %s, health %.0f -> %.0f\n", victim->type.name.c_str(), "hit for 2",
				first ? "lands" : "ignored", second ? "lands (wrong)" : "ignored", health_before, victim->health);
			printf("  0.5 s later: %.2f blocks further from the attacker\n", length(flat(victim->position - source)) - length(flat(start - source)));

			float path = 0.0f;
			int blocked_frames = 0;
			vec3 previous = victim->position;
			for (int i = 0; i < 150; ++i) {
				step(1);
				path += length(flat(victim->position - previous));
				previous = victim->position;
				if (victim->blocked) blocked_frames++;
			}
			printf("  panicking: %.2f blocks/s over 2.5 s (walks at %.2f), turned back by a drop, water or wall in %d of 150 frames\n",
				path / 2.5f, victim->type.walk_speed, blocked_frames);

			int hits = 1;
			while (!victim->dying() && hits < 50) {
				step(36);
				if (victim->hurt(2.0f, source)) hits++;
			}
			printf("  died after %d hits of 2 (%.0f health)\n", hits, victim->type.max_health);
			step(20);
			shoot(*victim, "dying", mobs);

			//a horse put down near the player: it should wander without clipping into anything
			for (const MobType& t : mob_types()) {
				if (t.name != "horse") continue;
				Mob* horse = nullptr;
				for (int r = 3; r <= 20 && horse == nullptr; ++r) {
					for (int a = 0; a < 16 && horse == nullptr; ++a) {
						int x = (int)std::round(position.x + cos(a * 0.3927f) * r), z = (int)std::round(position.z + sin(a * 0.3927f) * r);
						int h = get_terrain_generator().sample_height(x, z);
						Block* top = cm.get_block_worldspace(vec3(x, h, z));
						if (h <= water_level || top == nullptr || !is_solid(top->type)) continue;
						horse = mobs.add(t, vec3(x, h + 0.51f, z));
					}
				}
				if (horse == nullptr) {
					printf("  no room to put a horse down\n");
					break;
				}
				vec3 start_at = horse->position;
				long horse_frames = 0, horse_overlap = 0;
				float travelled = 0.0f;
				vec3 last = horse->position;
				for (int f = 0; f < 60 * 30; ++f) {
					step(1);
					horse_frames++;
					if (penetrates(*horse)) horse_overlap++;
					travelled += length(flat(horse->position - last));
					last = horse->position;
				}
				printf("  a horse over 30 s: walked %.1f blocks, ends %.1f from where it started, hitbox in a block in %ld of %ld frames\n",
					travelled, length(flat(horse->position - start_at)), horse_overlap, horse_frames);
				shoot(*horse, "horse", mobs);
			}
			int frames_after = 20;
			while (alive(victim) && frames_after < 600) {
				step(1);
				frames_after++;
			}
			printf("  removed %.2f s after dying\n", frames_after / 60.0f);

			int tested = 0;
			for (const auto& m : mobs.all()) {
				if (tested == 3) break;
				Mob* faller = m.get();
				if (faller->dying() || faller->hurt_time > 0.0f || faller->panic_time > 0.0f || faller->type.lives != habitat::land) continue;
				float drop = 0.0f;
				for (float lift : { 8.0f, 7.0f, 6.0f, 5.0f }) {
					faller->position.y += lift;
					if (faller->fits()) { drop = lift; break; }
					faller->position.y -= lift;
				}
				if (drop == 0.0f) continue;
				tested++;
				float health_before_fall = faller->health, y_top = faller->position.y;
				faller->velocity = vec3(0.0f);
				//measured at touchdown, before it can walk up or down anything
				bool falling = false;
				for (int i = 0; i < 120; ++i) {
					step(1);
					if (falling && faller->velocity.y == 0.0f) break;
					falling = faller->velocity.y < 0.0f;
				}
				float fell = y_top - faller->position.y;
				printf("  %s fell %.2f blocks: health %.0f -> %.0f (expected %.0f)\n", faller->type.name.c_str(), fell,
					health_before_fall, faller->health, health_before_fall - glm::max(0.0f, std::floor(fell + 0.01f - Mob::safe_fall)));
			}
		}
		//the sea: move to the nearest deep water, let swimmers settle in, then come back
		printf("\naquatic mobs (3600 frames simulated at 60 fps)\n");
		ivec2 sea = ivec2(0);
		bool found_sea = false;
		for (int r = 16; r <= 3000 && !found_sea; r += 16) {
			for (int a = 0; a < 48 && !found_sea; ++a) {
				int x = (int)(cos(a * 0.1309f) * r), z = (int)(sin(a * 0.1309f) * r);
				if (get_terrain_generator().sample_height(x, z) <= water_level - 7) {
					sea = ivec2(x, z);
					found_sea = true;
				}
			}
		}
		if (!found_sea) printf("  no deep water within 3000 blocks\n");
		auto load_around = [&](vec3 where) {
			position = where;
			float budget = terrain.build_budget_ms;
			terrain.build_budget_ms = 1e9f;
			for (int i = 0; i < 400; ++i) {
				terrain.update_chunks();
				terrain.draw(mat4(1.0f));
				if (i > 2 && terrain.stats.chunks_pending == 0) break;
			}
			terrain.build_budget_ms = budget;
		};
		if (found_sea) {
			vec3 home = position;
			load_around(vec3(sea.x + 0.5f, water_level + 2.0f, sea.y + 0.5f));
			printf("  sea at (%d, %d), %d blocks deep\n", sea.x, sea.y, water_level - get_terrain_generator().sample_height(sea.x, sea.y));

			MobManager sea_mobs;
			MobContext sea_context;
			sea_context.player_eye = position;
			long swimmer_frames = 0, beached_frames = 0, sea_overlap = 0, sea_frames = 0;
			int breaches = 0, hauled_out = 0, went_in = 0;
			map<const Mob*, int> medium; //seals: 1 water, 2 land
			map<const Mob*, bool> airborne;
			map<const Mob*, bool> counted;
			map<string, int> spawned;
			double sea_update_ms = 0;
			for (int f = 0; f < 3600; ++f) {
				double a = now_ms();
				sea_mobs.update(1.0f / 60.0f, sea_context);
				sea_update_ms += now_ms() - a;
				for (const auto& m : sea_mobs.all()) {
					if (!counted[m.get()]) {
						counted[m.get()] = true;
						spawned[m->type.name]++;
					}
					sea_frames++;
					if (penetrates(*m)) sea_overlap++;
					if (m->type.lives == habitat::water) {
						swimmer_frames++;
						if (!m->in_water) beached_frames++;
					}
					if (m->type.lives == habitat::amphibious) {
						int now = m->in_water ? 1 : (m->on_ground() ? 2 : 0);
						int& last = medium[m.get()];
						if (now != 0) {
							if (last == 1 && now == 2) hauled_out++;
							if (last == 2 && now == 1) went_in++;
							last = now;
						}
					}
					if (m->type.name == "dolphin") {
						bool up = m->feet().y > water_level + 0.5f;
						if (up && !airborne[m.get()]) breaches++;
						airborne[m.get()] = up;
					}
				}
			}
			printf("  spawned:");
			for (const auto& entry : spawned) printf(" %d %s", entry.second, entry.first.c_str());
			printf("\n  swimmers out of the water in %ld of %ld frames, hitbox in a block in %ld of %ld mob-frames\n", beached_frames, swimmer_frames, sea_overlap, sea_frames);
			printf("  dolphin leaps clear of the surface: %d; seals hauling out: %d, slipping back in: %d\n", breaches, hauled_out, went_in);
			int seals = 0, seals_swimming = 0;
			for (const auto& m : sea_mobs.all()) {
				if (m->type.lives != habitat::amphibious) continue;
				seals++;
				if (m->in_water) seals_swimming++;
			}
			printf("  seals in the water at the end: %d of %d\n", seals_swimming, seals);
			printf("  cost per frame: update %.3f ms for %zu mobs\n", sea_update_ms / 3600, sea_mobs.all().size());

			//nearest of each kind, then a fish stranded on the shore
			map<string, Mob*> sea_subjects;
			for (const auto& m : sea_mobs.all()) {
				Mob*& best = sea_subjects[m->type.name];
				if (best == nullptr || distance(m->position, position) < distance(best->position, position)) best = m.get();
			}
			mat4 no_shadow = mat4(0.0f);
			no_shadow[3] = vec4(0.0f, 0.0f, 2.0f, 1.0f);
			shot_light = no_shadow;
			for (const auto& entry : sea_subjects) shoot(*entry.second, "sea_" + entry.first, sea_mobs);

			Mob* fish = nullptr;
			for (const auto& m : sea_mobs.all()) {
				if (m->type.lives == habitat::water && m->type.name != "dolphin") { fish = m.get(); break; }
			}
			if (fish != nullptr) {
				bool placed = false;
				ivec3 at = ivec3(round(fish->position));
				for (int r = 1; r <= 64 && !placed; ++r) {
					for (int dx = -r; dx <= r && !placed; ++dx) {
						for (int dz = -r; dz <= r && !placed; dz += (std::abs(dx) == r ? 1 : 2 * r)) {
							int x = at.x + dx, z = at.z + dz;
							int h = get_terrain_generator().sample_height(x, z);
							Block* top = cm.get_block_worldspace(vec3(x, h, z));
							Block* above = cm.get_block_worldspace(vec3(x, h + 1, z));
							if (h <= water_level || top == nullptr || !is_solid(top->type) || above == nullptr || above->type != none) continue;
							fish->position = vec3(x, h + 0.51f + fish->size.y * 0.5f, z);
							fish->velocity = vec3(0.0f);
							placed = fish->fits();
						}
					}
				}
				if (placed) {
					int hops = 0;
					float died_at = -1.0f;
					bool was_rising = false;
					for (int f = 0; f < 600 && died_at < 0.0f; ++f) {
						sea_mobs.update(1.0f / 60.0f, sea_context);
						bool rising = fish->velocity.y > 2.0f;
						if (rising && !was_rising) hops++;
						was_rising = rising;
						if (fish->dying()) died_at = f / 60.0f;
						if (f == 30) shoot(*fish, "sea_stranded", sea_mobs);
					}
					printf("  a %s put on dry land: %d flops, %s\n", fish->type.name.c_str(), hops,
						died_at >= 0.0f ? ("suffocated after " + to_string(died_at).substr(0, 4) + " s").c_str() : "still alive after 10 s");
				}
			}
			auto type_named = [](const string& name) -> const MobType* {
				for (const MobType& t : mob_types()) if (t.name == name) return &t;
				return nullptr;
			};
			auto block_type_at = [&](int x, int y, int z) {
				Block* b = cm.get_block_worldspace(vec3(x, y, z));
				return b != nullptr ? b->type : bedrock;
			};

			//a seal put in the water a few blocks off a shore: how long until it climbs out
			bool shore_found = false;
			for (int r = 2; r <= 40 && !shore_found; ++r) {
				for (int dx = -r; dx <= r && !shore_found; ++dx) {
					for (int dz = -r; dz <= r && !shore_found; dz += (std::abs(dx) == r ? 1 : 2 * r)) {
						int x = sea.x + dx, z = sea.y + dz, y = water_level;
						if (!is_solid(block_type_at(x, y, z)) || block_type_at(x, y + 1, z) != none) continue;
						for (ivec2 step : { ivec2(3, 0), ivec2(-3, 0), ivec2(0, 3), ivec2(0, -3) }) {
							int wx = x + step.x, wz = z + step.y;
							if (block_type_at(wx, y, wz) != water || block_type_at(wx, y - 1, wz) != water) continue;
							for (const char* kind : { "seal", "snow_seal" }) {
								Mob* seal = sea_mobs.add(*type_named(kind), vec3(wx, y - 1.0f, wz));
								if (seal == nullptr) continue;
								shore_found = true;
								float out_at = -1.0f;
								for (int f = 0; f < 60 * 60 && out_at < 0.0f; ++f) {
									sea_mobs.update(1.0f / 60.0f, sea_context);
									if (!seal->in_water && seal->on_ground()) out_at = f / 60.0f;
								}
								printf("  a %s put in the water 3 blocks off a shore: %s\n", kind,
									out_at >= 0.0f ? ("hauled out after " + to_string(out_at).substr(0, 4) + " s").c_str() : "still swimming after 60 s");
								if (out_at >= 0.0f) {
									shoot(*seal, string("sea_") + kind + "_ashore", sea_mobs);
									seal->head_yaw = seal->head_pitch = 0.0f;
									shoot(*seal, string("sea_") + kind + "_face", sea_mobs, nullptr, 2);
								}
							}
							if (shore_found) break;
						}
					}
				}
			}

			//a dolphin in open water: how often it leaps
			Mob* dolphin = sea_mobs.add(*type_named("dolphin"), vec3(sea.x, water_level - 3.0f, sea.y));
			if (dolphin != nullptr) {
				int leaps = 0;
				bool up = false;
				for (int f = 0; f < 60 * 60; ++f) {
					sea_mobs.update(1.0f / 60.0f, sea_context);
					bool clear = dolphin->feet().y > water_level + 0.5f;
					if (clear && !up) leaps++;
					up = clear;
					if (leaps == 1 && clear && dolphin->velocity.y < 0.0f && dolphin->velocity.y > -2.0f) shoot(*dolphin, "sea_dolphin_leap", sea_mobs);
				}
				printf("  a dolphin in open water: %d leaps in 60 s, %s\n", leaps, dolphin->in_water ? "back in the water" : "out of the water");
			}

			shot_light = shadow_light;
			load_around(home);
		}
		//commands, riding and pet dragons, on open plains so trees don't get in the way
		printf("\nriding and dragons\n");
		ivec2 field = ivec2(0);
		bool found_field = false;
		for (int r = 0; r <= 2000 && !found_field; r += 24) {
			for (int a = 0; a < 48 && !found_field; ++a) {
				int x = (int)(cos(a * 0.1309f) * r), z = (int)(sin(a * 0.1309f) * r);
				ClimateSample c = get_terrain_generator().sample_climate(x, z);
				biome_id b = select_biome(c, water_level);
				if (b != biome_id::plains && b != biome_id::savanna) continue;
				int h = get_terrain_generator().sample_height(x, z);
				bool flat = h > water_level + 1;
				for (int dx = -8; dx <= 40 && flat; dx += 4) {
					for (int dz = -8; dz <= 8 && flat; dz += 4) {
						flat = std::abs(get_terrain_generator().sample_height(x + dx, z + dz) - h) <= 3;
					}
				}
				if (flat) {
					field = ivec2(x, z);
					found_field = true;
				}
			}
		}
		if (!found_field) printf("  no open plains within 2000 blocks\n");
		else {
		vec3 field_home = position;
		load_around(vec3(field.x + 0.5f, get_terrain_generator().sample_height(field.x, field.y) + 1.5f, field.y + 0.5f));
		printf("  on plains at (%d, %d)\n", field.x, field.y);
		mat4 field_light = mat4(0.0f);
		field_light[3] = vec4(0.0f, 0.0f, 2.0f, 1.0f);
		shot_light = field_light;
		{
			vec3 eye = position + vec3(0.0f, 0.72f, 0.0f);
			vec3 look = vec3(1.0f, 0.0f, 0.0f);
			MobManager pets;
			MobContext pet_context;
			pet_context.player_eye = eye;
			for (const char* line : { "/summon haku", "/summon horse", "/summon unicorn", "/fly" }) {
				printf("  %-16s -> %s\n", line, run_command(line, pets, eye, look).c_str());
			}
			Mob* eastern = nullptr;
			Mob* steed = nullptr;
			for (const auto& m : pets.all()) {
				if (m->type.name == "eastern_dragon") eastern = m.get();
				if (m->type.name == "horse") steed = m.get();
			}
			auto settle = [&](int frames) { for (int i = 0; i < frames; ++i) pets.update(1.0f / 60.0f, pet_context); };
			settle(60);
			if (eastern != nullptr) shoot(*eastern, "dragon_eastern_idle", pets);

			//ride a mob with fixed controls for a while; returns how far it went
			auto ride_for = [&](Mob& mount, RideInput input, int frames) {
				vec3 start = mount.position;
				mount.ridden = true;
				for (int i = 0; i < frames; ++i) {
					mount.steer(input);
					settle(1);
				}
				return mount.position - start;
			};
			Mob rider_model(rider_type(), vec3(0.0f), 0u);
			auto seat_rider = [&](const Mob& mount) {
				rider_model.position = mount.seat() + vec3(0.0f, rider_model.size.y * 0.5f, 0.0f);
				rider_model.yaw = mount.yaw;
				rider_model.pitch = mount.pitch;
				rider_model.bank = mount.bank;
			};

			if (steed != nullptr) {
				RideInput gallop;
				gallop.forward = 1.0f;
				gallop.sprint = true;
				//along +x, the stretch the field search checked is open
				gallop.yaw = 1.5708f;
				gallop.look = vec3(1.0f, 0.0f, 0.0f);
				vec3 moved = ride_for(*steed, gallop, 180);
				float top = steed->position.y;
				RideInput jump = gallop;
				jump.up = true;
				jump.forward = 0.0f;
				for (int i = 0; i < 40; ++i) {
					steed->steer(jump);
					settle(1);
					top = glm::max(top, steed->position.y);
					jump.up = i < 2;
				}
				printf("  horse ridden at a gallop for 3 s: %.1f blocks; a jump rises %.2f blocks\n", length(vec2(moved.x, moved.z)), top - (steed->position.y));
				seat_rider(*steed);
				shoot(*steed, "ride_horse", pets, &rider_model);
				steed->ridden = false;
			}

			if (eastern != nullptr) {
				RideInput cruise;
				cruise.forward = 1.0f;
				cruise.yaw = eastern->yaw - 0.8f;
				cruise.look = normalize(vec3(sin(cruise.yaw), 0.3f, cos(cruise.yaw)));
				vec3 moved = ride_for(*eastern, cruise, 180);
				printf("  eastern dragon ridden for 3 s: %.1f blocks, %.1f up\n", length(moved), moved.y);
				seat_rider(*eastern);
				shoot(*eastern, "ride_eastern_flying", pets, &rider_model);
				eastern->ridden = false;
				float before = eastern->position.y;
				settle(180);
				printf("  let go, it hovers: %.2f blocks of drift in height over 3 s (flying: %s)\n", eastern->position.y - before, eastern->flying ? "yes" : "no");
			}

			//walk the player 40 blocks away; the pets should come after them
			pet_context.player_eye = eye + vec3(40.0f, 6.0f, 0.0f);
			for (Mob* pet : { eastern }) {
				if (pet == nullptr) continue;
				float start = distance(pet->position, pet_context.player_eye);
				int frames = 0;
				while (distance(pet->position, pet_context.player_eye) > 6.0f && frames < 60 * 20) {
					settle(1);
					frames++;
				}
				printf("  %s %.0f blocks from the player: %s after %.1f s\n", pet->type.name.c_str(), start,
					distance(pet->position, pet_context.player_eye) <= 6.0f ? "caught up" : "still behind", frames / 60.0f);
			}
		}

		shot_light = shadow_light;
		load_around(field_home);
		}
		//the polar sea: ice floes, and snow seals on and around them
		printf("\npolar sea (3600 frames simulated at 60 fps)\n");
		ivec2 floe = ivec2(0);
		bool found_floe = false;
		for (int r = 16; r <= 4000 && !found_floe; r += 16) {
			for (int a = 0; a < 64 && !found_floe; ++a) {
				int x = (int)(cos(a * 0.0982f) * r), z = (int)(sin(a * 0.0982f) * r);
				ClimateSample c = get_terrain_generator().sample_climate(x, z);
				if (select_biome(c, water_level) == biome_id::frozen_ocean && get_terrain_generator().has_ice(x, z, c.temperature)) {
					floe = ivec2(x, z);
					found_floe = true;
				}
			}
		}
		if (!found_floe) printf("  no ice within 4000 blocks\n");
		else {
			vec3 home = position;
			load_around(vec3(floe.x + 0.5f, water_level + 2.0f, floe.y + 0.5f));
			int ice_cells = 0, open_cells = 0;
			for (int dx = -48; dx <= 48; ++dx) {
				for (int dz = -48; dz <= 48; ++dz) {
					Block* b = cm.get_block_worldspace(vec3(floe.x + dx, water_level, floe.y + dz));
					if (b == nullptr) continue;
					if (b->type == ice) ice_cells++;
					if (b->type == water) open_cells++;
				}
			}
			printf("  floe at (%d, %d); sea surface within 48 blocks: %d ice, %d open water\n", floe.x, floe.y, ice_cells, open_cells);

			MobManager polar;
			MobContext polar_context;
			polar_context.player_eye = position;
			const MobType* snow_seal_type = nullptr;
			for (const MobType& t : mob_types()) if (t.name == "snow_seal") snow_seal_type = &t;
			Mob* watched = polar.add(*snow_seal_type, vec3(floe.x, water_level + 0.51f, floe.y));

			map<const Mob*, bool> seen_polar;
			map<string, int> polar_spawned;
			long polar_frames = 0, polar_overlap = 0, on_ice = 0, seal_frames = 0;
			int dives = 0, climbs = 0;
			bool watched_in_water = false;
			for (int f = 0; f < 3600; ++f) {
				polar.update(1.0f / 60.0f, polar_context);
				for (const auto& m : polar.all()) {
					if (!seen_polar[m.get()]) {
						seen_polar[m.get()] = true;
						polar_spawned[m->type.name]++;
					}
					polar_frames++;
					if (penetrates(*m)) polar_overlap++;
					if (&m->type != snow_seal_type) continue;
					seal_frames++;
					Block* under = cm.get_block_worldspace(round(m->feet() - vec3(0.0f, 0.5f, 0.0f)));
					if (m->on_ground() && under != nullptr && under->type == ice) on_ice++;
				}
				if (watched != nullptr) {
					if (watched->in_water && !watched_in_water) dives++;
					if (!watched->in_water && watched_in_water && watched->on_ground()) climbs++;
					if (!watched->in_water && watched->on_ground()) watched_in_water = false;
					else if (watched->in_water) watched_in_water = true;
				}
			}
			printf("  spawned:");
			for (const auto& entry : polar_spawned) printf(" %d %s", entry.second, entry.first.c_str());
			printf("\n  snow seals on ice in %ld of %ld seal-frames; hitbox in a block in %ld of %ld mob-frames\n", on_ice, seal_frames, polar_overlap, polar_frames);
			printf("  the snow seal set on a floe: slipped into the water %d times, climbed back onto ice or shore %d times\n", dives, climbs);

			mat4 no_shadow = mat4(0.0f);
			no_shadow[3] = vec4(0.0f, 0.0f, 2.0f, 1.0f);
			shot_light = no_shadow;
			if (watched != nullptr) {
				shoot(*watched, "polar_snow_seal", polar);
				watched->head_yaw = watched->head_pitch = 0.0f;
				shoot(*watched, "polar_snow_seal_face", polar, nullptr, 2);
			}
			shot_light = shadow_light;
			load_around(home);
		}
		//the hot springs: where they are, what gets built, and Haku living at one
		printf("\nonsen\n");
		{
			vector<OnsenSite> sites = onsen::sites_near(vec3(0.0f), 3000.0f);
			float spacing = 0.0f;
			for (const OnsenSite& a : sites) {
				float closest = 1e9f;
				for (const OnsenSite& b : sites) {
					if (&a == &b) continue;
					closest = glm::min(closest, length(vec2(a.centre.x - b.centre.x, a.centre.z - b.centre.z)));
				}
				spacing += closest;
			}
			printf("  %zu onsen within 3000 blocks of the origin, %.0f blocks to the next one on average\n", sites.size(), sites.size() > 1 ? spacing / sites.size() : 0.0f);
			OnsenSite site;
			if (!onsen::nearest(vec3(0.0f), 3000.0f, site)) printf("  none to visit\n");
			else {
				vec3 onsen_home = position;
				load_around(vec3(site.centre) + vec3(0.5f, 3.0f, onsen::radius + 4.5f));
				printf("  nearest at %d %d %d, %.0f blocks from the origin\n", site.centre.x, site.centre.y, site.centre.z, length(vec2(site.centre.x, site.centre.z)));
				map<block_type, int> counts;
				int blocked_above = 0;
				for (int dx = -onsen::radius; dx <= onsen::radius; ++dx) {
					for (int dz = -onsen::radius; dz <= onsen::radius; ++dz) {
						for (int dy = -3; dy <= 16; ++dy) {
							Block* b = cm.get_block_worldspace(vec3(site.centre.x + dx, site.centre.y + dy, site.centre.z + dz));
							if (b == nullptr) continue;
							counts[b->type]++;
							//anything natural left standing on the terrace
							bool built = b->type == planks || b->type == red_lacquer || b->type == roof_tile || b->type == stone_brick
								|| b->type == wood || b->type == glowstone || b->type == stone || b->type == mossy_stone;
							if (dy >= 1 && dx * dx + dz * dz <= onsen::radius * onsen::radius && b->type != none && !built) blocked_above++;
						}
					}
				}
				printf("  built: %d hot water, %d glowstone, %d vermilion, %d roof tiles, %d planks, %d stone bricks; %d stray blocks left above the terrace\n",
					counts[water], counts[glowstone], counts[red_lacquer], counts[roof_tile], counts[planks], counts[stone_brick], blocked_above);

				MobManager spring;
				MobContext spring_context;
				spring_context.player_eye = position + vec3(0.0f, 0.72f, 0.0f);
				Mob* resident = nullptr;
				float furthest = 0.0f;
				for (int f = 0; f < 60 * 60; ++f) {
					spring.update(1.0f / 60.0f, spring_context);
					for (const auto& m : spring.all()) {
						if (m->has_home) resident = m.get();
					}
					if (resident != nullptr) furthest = glm::max(furthest, distance(resident->position, resident->home));
				}
				printf("  %s; over 60 s it strays at most %.1f blocks from its spring\n",
					resident != nullptr ? "Haku has moved in" : "no Haku came", furthest);

				mat4 no_shadow = mat4(0.0f);
				no_shadow[3] = vec4(0.0f, 0.0f, 2.0f, 1.0f);
				shot_light = no_shadow;
				custom_target = vec3(site.centre) + vec3(0.0f, 1.0f, -1.0f);
				custom_eye = vec3(site.centre) + vec3(9.0f, 9.0f, 20.0f);
				if (!spring.all().empty()) {
					const Mob& subject = resident != nullptr ? *resident : *spring.all().front();
					shoot(subject, "onsen_overview", spring, nullptr, 3);
					custom_eye = vec3(site.centre) + vec3(-14.0f, 16.0f, -4.0f);
					shoot(subject, "onsen_above", spring, nullptr, 3);
					custom_eye = vec3(site.centre) + vec3(0.5f, 2.5f, 13.0f);
					custom_target = vec3(site.centre) + vec3(0.5f, 2.0f, -6.0f);
					shoot(subject, "onsen_gate", spring, nullptr, 3);
				}
				shot_light = shadow_light;
				load_around(onsen_home);
			}
		}
	}

	printf("\nstreaming (walking east one chunk at a time, %.0f ms build budget)\n", 3.0f);
	terrain.build_budget_ms = 3.0f;
#ifdef _DEBUG
	//verifies the whole heap on every allocation, so a stray write is caught close to where it happens
	if (heap_check) _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF);
#else
	(void)heap_check;
#endif
	for (int step = 0; step < 4; ++step) {
		position.x += chunk_size;
		position.y = (float)get_terrain_generator().sample_height((int)position.x, (int)position.z) + 2.0f;
		int frames = 0, built = 0, generated = 0, worst_built = 0, worst_rebuilt = 0;
		double total_ms = 0, worst_ms = 0, worst_gen = 0, worst_build = 0, worst_rebuild = 0, worst_structures = 0;
		do {
			double a = now_ms();
			terrain.update_chunks();
			double ms = now_ms() - a;
			terrain.draw(mat4(1.0f));
			frames++;
			total_ms += ms;
			if (ms > worst_ms) {
				worst_ms = ms;
				worst_gen = terrain.stats.generate_ms;
				worst_build = terrain.stats.chunk_build_ms;
				worst_built = terrain.stats.chunks_built;
				worst_rebuild = terrain.stats.rebuild_ms;
				worst_structures = terrain.stats.structures_ms;
				worst_rebuilt = terrain.stats.chunks_rebuilt;
			}
			built += terrain.stats.chunks_built;
			generated += terrain.stats.chunks_generated;
		} while (terrain.stats.chunks_pending > 0 && frames < 500);
		printf("  step %d: %d generated, %d meshes built (incl. relights) over %d frames, update %.1f ms total\n",
			step + 1, generated, built, frames, total_ms);
		printf("          worst frame %.1f ms = generate %.1f + build %.1f (%d meshes) + structures %.1f + immediate rebuild %.1f (%d) + other %.1f\n",
			worst_ms, worst_gen, worst_build, worst_built, worst_structures, worst_rebuild, worst_rebuilt, worst_ms - worst_gen - worst_build - worst_structures - worst_rebuild);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
