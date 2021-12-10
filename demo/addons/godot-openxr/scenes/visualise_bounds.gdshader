shader_type spatial;
render_mode depth_test_disable;

uniform vec4 color : hint_color = vec4(1.0, 1.0, 1.0, 0.25);
uniform vec3 scale = vec3(2.0, 2.0, 2.0);

varying vec3 v;

void vertex() {
	v = VERTEX.xyz * scale;

	// workaround for weird rounding bug when we're exactly at 1.0
	v += vec3(0.001, 0.001, 0.001);
}

void fragment() {
	float f1 = v.x - mod(v.x, 1.0);
	float f2 = v.y - mod(v.y, 1.0);
	float f3 = v.z - mod(v.z, 1.0);

	vec3 c = mod(f1+f2+f3, 2.0) * color.rgb;

	ALBEDO = c;
	ALPHA = color.a;
}
