#include "Shader.h"

/*
	reads file and retrieves its data
*/
string read_file(const char* filename) {
	ifstream in(filename, ios::binary);
	if (in) {
		string contents;
		in.seekg(0, ios::end);
		contents.resize(in.tellg());
		in.seekg(0, ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	throw errno;
}

/*
	checks error in vertex and fragment shader
*/
void check_errors(GLuint vertex, GLuint fragment, GLuint program) {
	int success;
	char infoLog[512];
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		cout << "ERROR: vertex shader compilation failed\n" << infoLog << endl;
	}
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		cout << "ERROR: fragment shader compilation failed\n" << infoLog << endl;
	}
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		cout << "ERROR: shader program link failed\n" << infoLog << endl;
	}
}

/*
	creates a shader object that couples a vertex and a fragment shader
*/
Shader::Shader(const char* vertex_file, const char* fragment_file) {
	string vertex_code = read_file(vertex_file);
	string frag_code = read_file(fragment_file);

	const char* vertex_src = vertex_code.c_str();
	const char* frag_src = frag_code.c_str();

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_src, NULL);
	glCompileShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &frag_src, NULL);
	glCompileShader(fragment_shader);

	id = glCreateProgram();
	glAttachShader(id, vertex_shader);
	glAttachShader(id, fragment_shader);
	glLinkProgram(id);

	check_errors(vertex_shader, fragment_shader, id);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

GLuint Shader::get_id()
{
	return id;
}

/*
	activates a shader
*/
void Shader::activate() {
	glUseProgram(id);
}

/*
	sends one integer uniform value.
	normally used for sending a boolean value or telling which texture slot to use.
*/
void Shader::set_uniform_1i(const char* uniform_name, GLint value) {
	glUniform1i(glGetUniformLocation(id, uniform_name), value);
}

/*
	sends one float uniform value
*/
void Shader::set_uniform_1f(const char* uniform_name, GLfloat value) {
	glUniform1f(glGetUniformLocation(id, uniform_name), value);
}

/*
	sends one or more vec4 uniform value.
	normally used for sending a colour.
*/
void Shader::set_uniform_4f(const char* uniform_name, GLsizei count, vec4 value) {
	glUniform4fv(glGetUniformLocation(id, uniform_name), count, value_ptr(value));
}

/*
	sends one or more mat4 uniform value.
	normally used for sending a camera matrix.
*/
void Shader::set_uniform_mat4f(const char* uniform_name, GLsizei count, GLboolean tranpose, mat4 value) {
	glUniformMatrix4fv(glGetUniformLocation(id, uniform_name), count, tranpose, value_ptr(value));
}

/*
	destroys a shader instance
*/
void Shader::destroy() {
	glDeleteProgram(id);
}