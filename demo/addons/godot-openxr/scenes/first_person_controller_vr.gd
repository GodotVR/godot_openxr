extends ARVROrigin

signal initialised
signal failed_initialisation

# Also add signals so we can have other parts of the application react to this.
signal session_begun
signal session_ending
signal focused_state
signal visible_state
signal pose_recentered

export var auto_initialise = true
export var start_passthrough = false
export (NodePath) var viewport = null

var interface : ARVRInterface

func get_interface() -> ARVRInterface:
	return interface

func _ready():
	if auto_initialise:
		initialise()

func initialise() -> bool:
	if interface:
		# we are already initialised
		return true

	interface = ARVRServer.find_interface("OpenXR")
	if interface and interface.initialize():
		print("OpenXR Interface initialized")

		# Find the viewport we're using to render our XR output
		var vp : Viewport = _get_xr_viewport()

		# Start passthrough?
		_start_passthrough()

		# Connect to our plugin signals
		_connect_plugin_signals()

		# Change our viewport so it is tied to our ARVR interface and renders to our HMD
		vp.arvr = true

		# We can't set keep linear yet because we won't know the correct value until after our session has begun.

		# increase our physics engine update speed
		var refresh_rate = $Configuration.get_refresh_rate()
		if refresh_rate == 0:
			# Only Facebook Reality Labs supports this at this time
			print("No refresh rate given by XR runtime")

			# Use something sufficiently high
			Engine.iterations_per_second = 144
		else:
			print("HMD refresh rate is set to " + str(refresh_rate))

			# Match our physics to our HMD
			Engine.iterations_per_second = refresh_rate

		emit_signal("initialised")
		return true
	else:
		emit_signal("failed_initialisation")
		return false

func _get_xr_viewport() -> Viewport:
	if viewport:
		var vp : Viewport = get_node(viewport)
		return vp
	else:
		return get_viewport()

func _start_passthrough():
	if start_passthrough:
		# make sure our viewports background is transparent
		_get_xr_viewport().transparent_bg = true

		# enable our passthrough
		$Configuration.start_passthrough()

func _connect_plugin_signals():
	ARVRServer.connect("openxr_session_begun", self, "_on_openxr_session_begun")
	ARVRServer.connect("openxr_session_ending", self, "_on_openxr_session_ending")
	ARVRServer.connect("openxr_focused_state", self, "_on_openxr_focused_state")
	ARVRServer.connect("openxr_visible_state", self, "_on_openxr_visible_state")
	ARVRServer.connect("openxr_pose_recentered", self, "_on_openxr_pose_recentered")

func _on_openxr_session_begun():
	print("OpenXR session begun")

	var vp : Viewport = _get_xr_viewport()
	if vp:
		# Our interface will tell us whether we should keep our render buffer in linear color space
		vp.keep_3d_linear = $Configuration.keep_3d_linear()

	emit_signal("session_begun")

func _on_openxr_session_ending():
	print("OpenXR session ending")
	emit_signal("session_ending")

func _on_openxr_focused_state():
	print("OpenXR focused state")
	emit_signal("focused_state")

func _on_openxr_visible_state():
	print("OpenXR visible state")
	emit_signal("visible_state")

func _on_openxr_pose_recentered():
	print("OpenXR pose recentered")
	emit_signal("pose_recentered")
