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

	double now_ms() {
		return chrono::duration<double, milli>(chrono::steady_clock::now().time_since_epoch()).count();
	}

	void set_view_uniforms(ShaderManager& sm, const mat4& view_projection, vec3 eye) {
		const vec3 sun = normalize(vec3(-0.4f, -0.8f, -0.3f));
		Shader* shaders[3] = { &sm.default_shader, &sm.wave_shader, &sm.foliage_shader };
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
		const int N = std::min<int>(80, (int)loaded.size());
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
		printf("  build_mesh %.3f ms per chunk (single thread), upload %.3f ms per chunk\n", mesh_ms / N, upload_ms / N);
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
