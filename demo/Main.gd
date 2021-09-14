extends Node3D

func _ready():
	# Start our OpenXR session
	$FPSController.initialise()

	# Just for testing, output the enabled extensions
	var interface : XRInterfaceOpenXR = $FPSController.get_interface()
	if interface:
		print("Enabled extensions: " + str(interface.get_enabled_extensions()))
		print("Supported refresh rates: " + str(interface.get_available_refresh_rates()))
		print("Supported color spaces: " + str(interface.get_available_color_spaces()))
		print("Current color space: " + str(interface.get_color_space()))

func _process(delta):
	# Test for escape to close application, space to reset our reference frame
	if (Input.is_key_pressed(KEY_ESCAPE)):
		get_tree().quit()
	elif (Input.is_key_pressed(KEY_SPACE)):
		# Calling center_on_hmd will cause the XRServer to adjust all tracking data so the player is centered on the origin point looking forward
		XRServer.center_on_hmd(true, true)

	# We minipulate our origin point to move around. Note that with roomscale tracking a little more then this is needed
	# because we'll rotate around our origin point, not around our player. But that is a subject for another day.
	if (Input.is_key_pressed(KEY_LEFT)):
		$FPSController.rotation.y += delta
	elif (Input.is_key_pressed(KEY_RIGHT)):
		$FPSController.rotation.y -= delta

	if (Input.is_key_pressed(KEY_UP)):
		$FPSController.position -= $FPSController.transform.basis.z * delta;
	elif (Input.is_key_pressed(KEY_DOWN)):
		$FPSController.position += $FPSController.transform.basis.z * delta;

	# this is a little dirty but we're going to just tie the trigger input of our controllers to their haptic output for testing
	$FPSController/LeftHandController.rumble = $FPSController/LeftHandController.get_value("analog_trigger")
	$FPSController/RightHandController.rumble = $FPSController/RightHandController.get_value("analog_trigger")
