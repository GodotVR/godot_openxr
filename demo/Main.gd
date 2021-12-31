extends Spatial

func _ready():
	var screen_instance = $Screen.get_scene_instance()
	if screen_instance:
		screen_instance.connect("toggle_bounds", self, "_on_toggle_bounds")

func _on_toggle_bounds():
	$FPSController/ShowBounds.visible = !$FPSController/ShowBounds.visible

func _process(delta):
	# Test for escape to close application, space to reset our reference frame
	if (Input.is_key_pressed(KEY_ESCAPE)):
		get_tree().quit()
	elif (Input.is_key_pressed(KEY_SPACE)):
		# Calling center_on_hmd will cause the ARVRServer to adjust all tracking data so the player is centered on the origin point looking forward
		ARVRServer.center_on_hmd(true, true)

	# We minipulate our origin point to move around. Note that with roomscale tracking a little more then this is needed
	# because we'll rotate around our origin point, not around our player. But that is a subject for another day.
	if (Input.is_key_pressed(KEY_LEFT)):
		$FPController.rotation.y += delta
	elif (Input.is_key_pressed(KEY_RIGHT)):
		$FPController.rotation.y -= delta

	if (Input.is_key_pressed(KEY_UP)):
		$FPController.translation -= $FPController.transform.basis.z * delta;
	elif (Input.is_key_pressed(KEY_DOWN)):
		$FPController.translation += $FPController.transform.basis.z * delta;

	# this is a little dirty but we're going to just tie the trigger input of our controllers to their haptic output for testing
	$FPController/LeftHandController.rumble = $FPController/LeftHandController.get_joystick_axis(JOY_VR_ANALOG_TRIGGER)
	$FPController/RightHandController.rumble = $FPController/RightHandController.get_joystick_axis(JOY_VR_ANALOG_TRIGGER)

func _on_FPController_initialised():
	# Just for testing, output the enabled extensions
	print("Enabled extensions: " + str($FPController/Configuration.get_enabled_extensions()))
	print("Supported refresh rates: " + str($FPController/Configuration.get_available_refresh_rates()))
	print("Supported color spaces: " + str($FPController/Configuration.get_available_color_spaces()))
	print("Current color space: " + str($FPController/Configuration.get_color_space()))

func _on_FPController_failed_initialisation():
	# exit our app
	get_tree().quit()
