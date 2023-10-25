$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);

void main() {
	gl_FragColor = texture2D(s_tex, v_texcoord0); // * v_color0; 
	//gl_FragColor = vec4(v_texcoord0.x, v_texcoord0.y, 0.0, 1.0); // * v_color0; 
}