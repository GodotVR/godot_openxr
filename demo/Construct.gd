extends Node

var target_size : Vector2

func _reposition():
	if (target_size.x ==0 or target_size.y == 0):
		return

	var window_size = OS.window_size

	# scale up to fit width
	var new_size = target_size * (window_size.x / target_size.x)

	if new_size.y < window_size.y:
		new_size = target_size * (window_size.y / target_size.y)

	$HMDPreview.rect_size = new_size

	var delta = new_size - window_size
	$HMDPreview.rect_position = -delta * 0.5

# Called when the node enters the scene tree for the first time.
func _ready():
	var vp = get_viewport()
	
	# Make sure we don't render 3D to our main viewport
	vp.disable_3d = true

	if OS.get_name() == "Android":
		# Our main viewport is not used, OpenXR handles the display
		$HMDPreview.visible = false

		# Render just once in case its used until OpenXR is all setup
		vp.render_target_update_mode = Viewport.UPDATE_ONCE
	else:
		# Setup our viewport texture
		$HMDPreview.texture = $HMDViewport.get_texture()

		# Alert on size changes
		get_tree().get_root().connect("size_changed", self, "_on_window_size_change")
		_on_window_size_change();

func _on_window_size_change():
	_reposition()

func _on_FPController_initialised():
	target_size = $HMDViewport/Main/FPController.get_interface().get_render_targetsize()
	_reposition()
