extends XROrigin3D

@export var viewport : NodePath = null

var interface : XRInterfaceOpenXR

func get_interface() -> XRInterfaceOpenXR:
	return interface

func initialise() -> bool:
	interface = XRServer.find_interface("OpenXR")
	if interface and interface.initialize():
		print("OpenXR Interface initialized")

		# Comment to disable passthrough
		$Configuration.start_passthrough()

		# Connect to our plugin signals
		_connect_plugin_signals()

		var vp : Viewport = null
		if viewport:
			vp = get_node(viewport)

		if !vp:
			vp = get_viewport()

		# Change our viewport so it is tied to our ARVR interface and renders to our HMD
		vp.use_xr = true

		# Our interface will tell us whether we should keep our render buffer in linear color space
		# If true our preview will be darker.
		vp.keep_3d_linear = interface.keep_3d_linear()

		# increase our physics engine update speed
		var refresh_rate = interface.get_refresh_rate()
		if refresh_rate == 0:
			# Only Facebook Reality Labs supports this at this time
			print("No refresh rate given by XR runtime")

			# Use something sufficiently high
			Engine.iterations_per_second = 144
		else:
			print("HMD refresh rate is set to " + str(refresh_rate))

			# Match our physics to our HMD
			Engine.iterations_per_second = refresh_rate

		# $Left_hand.set_physics_process(true)
		return true
	else:
		return false

func _connect_plugin_signals():
	XRServer.connect("openxr_session_begun", _on_openxr_session_begun)
	XRServer.connect("openxr_session_ending", _on_openxr_session_ending)
	XRServer.connect("openxr_focused_state", _on_openxr_focused_state)
	XRServer.connect("openxr_visible_state", _on_openxr_visible_state)
	XRServer.connect("openxr_pose_recentered", _on_openxr_pose_recentered)

func _on_openxr_session_begun():
	print("OpenXR session begun")

func _on_openxr_session_ending():
	print("OpenXR session ending")

func _on_openxr_focused_state():
	print("OpenXR focused state")

func _on_openxr_visible_state():
	print("OpenXR visible state")

func _on_openxr_pose_recentered():
	print("OpenXR pose recentered")
