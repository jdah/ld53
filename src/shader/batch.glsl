@module batch

@vs vs
in vec2 a_position;
in vec2 a_texcoord0;

in vec2 a_offset;
in vec2 a_scale;
in vec2 a_uvmin;
in vec2 a_uvmax;
in vec4 a_color;
in float a_z;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

out vec2 uv;
out vec4 color;
out float depth;

void main() {
    uv = a_uvmin + (a_texcoord0 * (a_uvmax - a_uvmin));
    color = a_color;
    depth = a_z;

    const vec2 ipos = a_offset + (a_scale * a_position);
    gl_Position = proj * model * view * vec4(ipos, 0.0, 1.0);
}
@end

@fs fs
uniform sampler2D tex;

in vec2 uv;
in vec4 color;
in float depth;

out vec4 frag_color;

void main() {
    frag_color = color * texture(tex, uv);
    if (frag_color.a < 0.0001) {
        discard;
    }
    gl_FragDepth = depth;
}
@end

@program batch vs fs
